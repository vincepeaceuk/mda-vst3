#!/usr/bin/env python3
"""Bundle verified VST3s and complete corresponding source for local delivery."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[1]
VERSION = "0.1.0"
JUCE_COMMIT = "d60b7a78030661574a615b586d9325ba7a7fa7dd"
MODELS = {"Piano": "mda Piano", "EPiano": "mda ePiano", "DX10": "mda DX10",
          "JX10": "mda JX10", "Leslie": "mda Leslie"}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--build-dir", type=Path, default=ROOT / "build")
    parser.add_argument("--output-dir", type=Path, default=ROOT / "dist")
    args = parser.parse_args()
    build, output = args.build_dir.resolve(), args.output_dir.resolve()
    subprocess.run(["python3", str(ROOT / "tools/bootstrap.py")], check=True)
    output.mkdir(parents=True, exist_ok=True)
    products = []
    architectures = set()
    sources = []
    for model, name in MODELS.items():
        bundle = build / f"MDA_{model}_artefacts/Release/VST3/{name}.vst3"
        binary = bundle / "Contents/MacOS" / name
        subprocess.run(["codesign", "--verify", "--strict", str(bundle)], check=True)
        arch = subprocess.check_output(["lipo", "-archs", str(binary)], text=True).split()
        architectures.add(tuple(arch))
        products.append({"name": name, "bundle": bundle.name, "architectures": arch,
                         "sha256": hashlib.sha256(binary.read_bytes()).hexdigest()})
        sources.append(bundle)
    if len(architectures) != 1:
        raise SystemExit("Plugin architectures differ; refusing a mixed package")
    arch = next(iter(architectures))
    label = "universal" if set(arch) == {"arm64", "x86_64"} else "-".join(arch)
    archive = output / f"MDA-Revival-{VERSION}-macOS-{label}.zip"
    if archive.exists():
        raise SystemExit(f"Archive already exists: {archive}; choose another output directory")
    with tempfile.TemporaryDirectory(prefix="mda-revival-package-") as temporary:
        package = Path(temporary) / f"MDA Revival {VERSION}"
        plugins = package / "VST3"
        plugins.mkdir(parents=True)
        for bundle in sources:
            destination = plugins / bundle.name
            shutil.copytree(bundle, destination)
            subprocess.run(["codesign", "--verify", "--strict", str(destination)], check=True)
        shutil.copy2(ROOT / "tools/Install.command", package / "Install.command")
        (package / "Install.command").chmod(0o755)
        for name in ("README.md", "LICENSE.md"):
            shutil.copy2(ROOT / name, package / name)
        shutil.copytree(ROOT / "licenses", package / "Licences")
        shutil.copytree(ROOT / "docs/screenshots", package / "Interface previews")
        source = package / "Source"
        source.mkdir()
        ignore = shutil.ignore_patterns("__pycache__", "*.pyc", ".DS_Store", ".git")
        for name in ("plugins", "shared", "tests", "tools", "docs", "licenses"):
            shutil.copytree(ROOT / name, source / name, ignore=ignore)
        for name in ("README.md", "LICENSE.md", "CMakeLists.txt", ".gitignore"):
            shutil.copy2(ROOT / name, source / name)
        juce = source / "deps/JUCE"
        shutil.copytree(ROOT / "deps/JUCE", juce, ignore=ignore)
        (juce / "MDA-PINNED-COMMIT.txt").write_text(JUCE_COMMIT + "\n")
        revision = subprocess.check_output(["git", "-C", str(ROOT), "rev-parse", "HEAD"], text=True).strip()
        dirty = bool(subprocess.check_output(["git", "-C", str(ROOT), "status", "--porcelain"], text=True).strip())
        manifest = {"version": VERSION, "sourceRevision": revision, "sourceHasUncommittedChanges": dirty,
                    "juce": {"version": "9.0.5", "commit": JUCE_COMMIT}, "plugins": products,
                    "signing": "ad hoc; not notarised", "licence": "AGPL-3.0-or-later with original MIT notices"}
        (package / "MANIFEST.json").write_text(json.dumps(manifest, indent=2) + "\n")
        subprocess.run(["ditto", "-c", "-k", "--keepParent", str(package), str(archive)], check=True)
    checksum = hashlib.sha256(archive.read_bytes()).hexdigest()
    archive.with_suffix(".zip.sha256").write_text(f"{checksum}  {archive.name}\n")
    print(f"Created {archive} ({archive.stat().st_size / 1048576:.1f} MiB)")
    print(f"SHA-256 {checksum}")


if __name__ == "__main__":
    main()
