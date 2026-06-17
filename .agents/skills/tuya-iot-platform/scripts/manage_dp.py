#!/usr/bin/env python3
"""
Manage DPs (功能点) for a Tuya IoT product via tuya-devplat-cli.

Subcommands:
    list      List DPs currently attached to the product
    catalog   Browse the standard DP catalog for the product's category
    add       Add standard DPs to the product (dry-run → confirm)
    remove    Remove one standard DP from the product (dry-run → confirm)
    validate  Validate the product's current DP configuration

Usage:
    python manage_dp.py list     --product-id <pid>
    python manage_dp.py catalog  --product-id <pid>
    python manage_dp.py catalog  --product-id <pid> --available
    python manage_dp.py add      --product-id <pid> --dp-ids 20,21,22
    python manage_dp.py remove   --product-id <pid> --dp-id 20
    python manage_dp.py validate --product-id <pid>
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
    data = run(["product", "dp-list", "--product-id", args.product_id, "--format", "json"])
    dps = data.get("data") or []
    if not dps:
        print("(no DPs attached)")
        return
    print(f"{'id':<6} {'code':<25} {'name':<15} {'required':<10} {'selected'}")
    print("-" * 65)
    for dp in dps:
        print(f"{dp.get('id',''):<6} {dp.get('code',''):<25} {dp.get('name',''):<15} "
              f"{str(dp.get('required','')):<10} {dp.get('selected','')}")


def cmd_catalog(args: argparse.Namespace) -> None:
    data = run(["product", "dp-standard-catalog", "--product-id", args.product_id, "--format", "json"])
    dps = (data.get("data") or {}).get("standardDps") or []
    if args.available:
        dps = [dp for dp in dps if not dp.get("selected")]
    if not dps:
        print("(no DPs to show)")
        return
    print(f"{'id':<6} {'code':<25} {'name':<15} {'mode':<5} {'type':<8} {'状态'}")
    print("-" * 72)
    for dp in dps:
        attached = "已挂载" if dp.get("selected") else "未挂载"
        print(f"{dp.get('id',''):<6} {dp.get('code',''):<25} {dp.get('name',''):<15} "
              f"{dp.get('mode',''):<5} {dp.get('type',''):<8} {attached}")


def cmd_add(args: argparse.Namespace) -> None:
    dp_ids = [int(x.strip()) for x in args.dp_ids.split(",")]
    print(f"Adding DPs {dp_ids} to product {args.product_id}")
    base_flags = [
        "product", "dp-add-standard",
        "--product-id", args.product_id,
        "--self-dps", json.dumps(dp_ids),
    ]
    run_write(base_flags)
    print("[ok]  DPs added")


def cmd_remove(args: argparse.Namespace) -> None:
    print(f"Removing DP {args.dp_id} from product {args.product_id}")
    base_flags = [
        "product", "dp-remove-standard",
        "--product-id", args.product_id,
        "--dp-id", str(args.dp_id),
    ]
    run_write(base_flags)
    print("[ok]  DP removed")


def cmd_validate(args: argparse.Namespace) -> None:
    data = run(["product", "dp-valid", "--product-id", args.product_id, "--format", "json"])
    dps = data.get("data") or []
    if not dps:
        print("(no DPs)")
        return
    print(f"{'id':<6} {'code':<25} {'name':<15} {'mode':<5} {'type'}")
    print("-" * 65)
    for dp in dps:
        print(f"{dp.get('id',''):<6} {dp.get('code',''):<25} {dp.get('name',''):<15} "
              f"{dp.get('mode',''):<5} {dp.get('type','')}")


# --- entry point ---

def main() -> None:
    parser = argparse.ArgumentParser(description="Manage DPs for a Tuya IoT product")
    sub = parser.add_subparsers(dest="subcmd", required=True)

    def add_pid(p: argparse.ArgumentParser) -> None:
        p.add_argument("--product-id", required=True, help="Product ID (PID)")

    p_list = sub.add_parser("list", help="List attached DPs")
    add_pid(p_list)

    p_catalog = sub.add_parser("catalog", help="Browse standard DP catalog")
    add_pid(p_catalog)
    p_catalog.add_argument("--available", action="store_true",
                           help="Show only DPs not yet attached")

    p_add = sub.add_parser("add", help="Add standard DPs")
    add_pid(p_add)
    p_add.add_argument("--dp-ids", required=True,
                       help="Comma-separated DP IDs to add (e.g. 20,21,22)")

    p_remove = sub.add_parser("remove", help="Remove one standard DP")
    add_pid(p_remove)
    p_remove.add_argument("--dp-id", required=True, type=int, help="DP ID to remove")

    p_validate = sub.add_parser("validate", help="Validate DP configuration")
    add_pid(p_validate)

    args = parser.parse_args()

    handlers = {
        "list": cmd_list,
        "catalog": cmd_catalog,
        "add": cmd_add,
        "remove": cmd_remove,
        "validate": cmd_validate,
    }
    handlers[args.subcmd](args)


if __name__ == "__main__":
    main()
