#!/usr/bin/env python3
"""
Manage Tuya IoT products via tuya-devplat-cli.

Subcommands:
    list    Search / list products
    detail  Get product details by PID
    create  Create a new product (dry-run → confirm)

Usage:
    python product.py list
    python product.py list --keyword "test"
    python product.py list --max 20
    python product.py detail --product-id <pid>
    python product.py create --name "my-product"
    python product.py create --name "my-socket" --category wf_ble_cz --solution-id 134001
"""

import argparse
import json
import os
import shlex
import subprocess
import sys


def _resolve_cli() -> list[str]:
    env = os.environ.get("TUYA_DEVPLAT_CLI", "")
    if env:
        return shlex.split(env)
    search = os.path.abspath(os.getcwd())
    for _ in range(10):
        name = "tuya-devplat-cli.cmd" if sys.platform == "win32" else "tuya-devplat-cli"
        candidate = os.path.join(search, ".tuyaopen", "ide", "bin", name)
        if os.path.isfile(candidate):
            return [candidate]
        parent = os.path.dirname(search)
        if parent == search:
            break
        search = parent
    return ["tuya-devplat-cli"]


CLI = _resolve_cli()
DEFAULT_CATEGORY = "wf_ble_qt"
DEFAULT_SOLUTION_ID = "10019526"


def run(args: list[str]) -> dict:
    result = subprocess.run(CLI + args, capture_output=True, text=True)
    try:
        data = json.loads(result.stdout)
    except json.JSONDecodeError:
        print(f"[error] CLI output is not valid JSON:\n{result.stdout}", file=sys.stderr)
        sys.exit(1)

    if not data.get("ok"):
        print(f"[error] {data.get('error', 'unknown error')} ({data.get('code', '')})", file=sys.stderr)
        if data.get("suggestion"):
            print(f"[hint]  {data['suggestion']}", file=sys.stderr)
        sys.exit(1)

    return data


def run_write(base_flags: list[str]) -> dict:
    """Run a mutating command: dry-run first, then confirm."""
    print("[1/2] dry-run ...")
    preview = run(base_flags + ["--dry-run", "--format", "json"])
    token = preview["data"]["confirm_token"]

    print(f"[2/2] confirm (token={token})")
    return run(base_flags + ["--confirm", token, "--format", "json"])


# --- subcommand handlers ---

def cmd_list(args: argparse.Namespace) -> None:
    flags = ["product", "list", "--format", "json", "--page-size", str(args.max)]
    if args.keyword:
        flags += ["--keyword", args.keyword]

    data = run(flags)
    products = (data.get("data") or {}).get("datas") or []

    if not products:
        print("(no products found)")
        return

    print(f"{'id':<20} {'name':<35} {'category':<12} {'status'}")
    print("-" * 80)
    for p in products:
        print(f"{p.get('id',''):<20} {p.get('name',''):<35} "
              f"{p.get('categoryName',''):<12} {p.get('developStatus','')}")


def cmd_detail(args: argparse.Namespace) -> None:
    data = run([
        "product", "detail",
        "--id", args.product_id,
        "--fields", "id,name,categoryName,categoryCode,communicationCodes,developStatus,gmtCreate",
        "--format", "json",
    ])
    p = data.get("data") or {}
    for key, val in p.items():
        print(f"{key}: {val}")


def cmd_create(args: argparse.Namespace) -> None:
    print(f"Creating product: name={args.name!r} category={args.category} solution-id={args.solution_id}")
    base_flags = [
        "product", "create-common",
        "--name", args.name,
        "--category", args.category,
        "--solution-id", args.solution_id,
    ]
    result = run_write(base_flags)
    pid = result["data"]["id"]
    print(f"[ok]  PID = {pid}")


# --- entry point ---

def main() -> None:
    parser = argparse.ArgumentParser(description="Manage Tuya IoT products")
    sub = parser.add_subparsers(dest="subcmd", required=True)

    p_list = sub.add_parser("list", help="Search / list products")
    p_list.add_argument("--keyword", default="", help="Search keyword")
    p_list.add_argument("--max", type=int, default=20, help="Max results (default: 20)")

    p_detail = sub.add_parser("detail", help="Get product details")
    p_detail.add_argument("--product-id", required=True, help="Product ID (PID)")

    p_create = sub.add_parser("create", help="Create a new product")
    p_create.add_argument("--name", required=True, help="Product name")
    p_create.add_argument("--category", default=DEFAULT_CATEGORY,
                          help=f"Frontend category code (default: {DEFAULT_CATEGORY})")
    p_create.add_argument("--solution-id", default=DEFAULT_SOLUTION_ID,
                          help=f"Solution ID (default: {DEFAULT_SOLUTION_ID})")

    args = parser.parse_args()
    {"list": cmd_list, "detail": cmd_detail, "create": cmd_create}[args.subcmd](args)


if __name__ == "__main__":
    main()
