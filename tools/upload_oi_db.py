#!/usr/bin/env python3
"""Upload the local OI archive into the LCA online database (GitHub releases).

Reads $HOME/.local/share/lca/archive/<olympiad>/<year>/ for each supported
olympiad, packs every year into a .tar.gz (preserving the relative layout) and
uploads it as a GitHub release asset. A release per olympiad is used (tag ==
olympiad name). An index.json at the repo root maps olympiads/years to asset
URLs and sizes, which is what `lca oi list/update` reads.

Requires: gh (authenticated), tar, python3.
"""

import argparse
import base64
import json
import os
import shutil
import subprocess
import sys
import tempfile

HOME = os.path.expanduser("~")
ARCHIVE = os.path.join(HOME, ".local", "share", "lca", "archive")
REPO = "TraianDanciu/lca-oi-db"

OLYMPIADS = [
    ("apio", "Asia-Pacific Informatics Olympiad"),
    ("baltoi", "Baltic Olympiad in Informatics"),
    ("ceoi", "Central European Olympiad in Informatics"),
    ("egoi", "European Girls' Olympiad in Informatics"),
    ("ejoi", "European Junior Olympiad in Informatics"),
    ("ioi", "International Olympiad in Informatics"),
    ("joi", "JOI Final (English)"),
    ("joioc", "JOI Open Contest"),
    ("joisc", "JOI Spring Camp (English)"),
]

ASSET_BASE = "https://github.com/{repo}/releases/download/{tag}/{name}"
INDEX_RAW = "https://raw.githubusercontent.com/{repo}/main/index.json"


def run(cmd, check=True):
    print("+ " + " ".join(cmd))
    result = subprocess.run(cmd, capture_output=True, text=True)
    if check and result.returncode != 0:
        sys.exit("command failed: {}\n{}".format(" ".join(cmd), result.stderr))
    return result


def gh(args, check=True):
    return run(["gh"] + args, check=check)


def pack_year(olympiad, year, out_dir):
    """Pack archive/<olympiad>/<year>/ into out_dir/<olympiad>-<year>.tar.gz.

    The tarball contains the year dir at its root, so extracting into
    archive/<olympiad>/ reproduces the original layout.
    """
    year_path = os.path.join(ARCHIVE, olympiad, year)
    if not os.path.isdir(year_path):
        return None
    name = "{}-{}.tar.gz".format(olympiad, year)
    dest = os.path.join(out_dir, name)
    with tempfile.TemporaryDirectory() as tmp:
        link = os.path.join(tmp, year)
        os.symlink(year_path, link)
        run(["tar", "-czhf", dest, "-C", tmp, year])
    return dest


def fetch_index(repo):
    result = gh(["api", "-H", "Accept: application/vnd.github+json",
                 "repos/{}/contents/index.json".format(repo)], check=False)
    if result.returncode != 0:
        return {}
    data = json.loads(result.stdout)
    return json.loads(base64.b64decode(data["content"]))


def put_index(repo, index):
    """Write index.json into the repo root via the GitHub contents API."""
    content = base64.b64encode(json.dumps(index, indent=2).encode()).decode()
    existing = gh(["api", "-H", "Accept: application/vnd.github+json",
                   "repos/{}/contents/index.json".format(repo)], check=False)
    if existing.returncode != 0:
        body = {
            "message": "Update OI database index",
            "content": content,
        }
    else:
        body = {
            "message": "Update OI database index",
            "content": content,
            "sha": json.loads(existing.stdout)["sha"],
        }
    gh(["api", "-X", "PUT", "-H", "Accept: application/vnd.github+json",
        "repos/{}/contents/index.json".format(repo),
        "-f", "message=" + body["message"],
        "-f", "content=" + body["content"]] + (["-f", "sha=" + body["sha"]] if "sha" in body else []))


def ensure_repo_initialized(repo):
    """GitHub releases in empty repos; create an initial README first."""
    info = json.loads(gh(["api", "repos/{}".format(repo)]).stdout)
    if info.get("isEmpty", False):
        content = base64.b64encode(
            b"# LCA online database\n\n"
            b"OI archives downloaded by `lca oi update`. One release per "
            b"olympiad, one asset (year tarball) per contest year. The list of "
            b"assets and sizes lives in index.json in the repo root.\n").decode()
        gh(["api", "-X", "PUT", "-H", "Accept: application/vnd.github+json",
            "repos/{}/contents/README.md".format(repo),
            "-f", "message=Initial commit", "-f", "content=" + content])
        print("[init] created initial README in " + repo)

def ensure_release(repo, tag):
    result = gh(["release", "view", tag, "--repo", repo], check=False)
    if result.returncode != 0:
        gh(["release", "create", tag, "--repo", repo,
            "--title", tag, "--notes", ""])


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--archive", default=ARCHIVE)
    parser.add_argument("--repo", default=REPO)
    parser.add_argument("--dry-run", action="store_true",
                        help="pack years and print index without uploading")
    args = parser.parse_args()

    archive = os.path.abspath(os.path.expanduser(args.archive))
    repo = args.repo

    if not args.dry_run:
        gh(["auth", "status"])
    ensure_repo_initialized(repo)

    olympiad_ids = [name for name, _ in OLYMPIADS]

    with tempfile.TemporaryDirectory() as work:
        pack_dir = os.path.join(work, "pack")
        os.makedirs(pack_dir)

        index = {} if args.dry_run else fetch_index(repo)
        if not isinstance(index, dict):
            index = {}

        for name, display in OLYMPIADS:
            year_dirs = []
            base = os.path.join(archive, name)
            if os.path.isdir(base):
                year_dirs = sorted(
                    d for d in os.listdir(base)
                    if os.path.isdir(os.path.join(base, d)))
            if not year_dirs:
                continue

            if not args.dry_run:
                ensure_release(repo, name)

            entry = index.get(name, {"display": display, "years": {}})
            entry["display"] = display
            entry["years"] = entry.get("years", {})

            for year in year_dirs:
                tarball = pack_year(name, year, pack_dir)
                if tarball is None:
                    continue
                size = os.path.getsize(tarball)
                asset = "{}-{}.tar.gz".format(name, year)
                url = ASSET_BASE.format(repo=repo, tag=name, name=asset)
                existing = entry["years"].get(year, {}).get("size")
                if args.dry_run:
                    print("[dry-run] would upload {} ({} MB)".format(
                        asset, round(size / 1e6, 1)))
                elif existing == size:
                    print("[skip] {} already at {} MB".format(
                        asset, round(size / 1e6, 1)))
                else:
                    run(["gh", "release", "upload", name, tarball,
                         "--repo", repo, "--clobber"])
                entry["years"][year] = {"size": size, "url": url}

            index[name] = entry

        print(json.dumps(index, indent=2))

        if args.dry_run:
            print("\n[dry-run] not writing to " + repo)
            return

        put_index(repo, index)
        print("\nUploaded index to " + repo)


if __name__ == "__main__":
    main()
