#!/usr/bin/env python3
"""Inventory report for independent SE backend API lowering.

A lowering marker indicates only code emission, NOT runtime parity.
Default prints a summary. --json outputs machine-readable data.
--require-complete returns nonzero unless every listed API has lowering.
"""
from __future__ import annotations

import argparse
import json
from pathlib import Path
import sys

ROOT = Path(__file__).resolve().parent.parent
INVENTORY = ROOT / "selfhost" / "extension_inventory.json"


def audit(path: Path) -> dict:
    source = json.loads(path.read_text(encoding="utf-8"))
    modules = source["modules"]
    rows = []
    for module in modules:
        members = module["members"]
        complete = sum(member.get("lowering_present") is True for member in members)
        missing = [member["name"] for member in members
                   if member.get("lowering_present") is not True]
        certified = sum(member.get("conformance") == "certified" for member in members)
        rows.append({
            "module": module["module"], "total": len(members),
            "lowering_present": complete,
            "missing": missing,
            "certified": certified
        })
    totals = {
        "modules": len(rows),
        "members": sum(row["total"] for row in rows),
        "lowerings": sum(row["lowering_present"] for row in rows),
        "missing": sum(len(row["missing"]) for row in rows),
        "certified": sum(row["certified"] for row in rows),
    }
    # The source's declared totals are sanity-checked, never silently trusted.
    if totals["modules"] != source.get("module_count"):
        raise ValueError("Inventory module count mismatch")
    if totals["members"] != source.get("member_count"):
        raise ValueError("Inventory member count mismatch")
    return {"summary": totals, "modules": rows}


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--inventory", type=Path, default=INVENTORY)
    parser.add_argument("--json", action="store_true")
    parser.add_argument("--require-complete", action="store_true",
                        help="exit 1 unless all API members are lowered and parity-certified")
    args = parser.parse_args()
    report = audit(args.inventory)
    if args.json:
        print(json.dumps(report, indent=2, ensure_ascii=False))
    else:
        total = report["summary"]
        print(f'SE selfhost API inventory: {total["modules"]} modules, '
              f'{total["members"]} members')
        print(f'Lowering present: {total["lowerings"]}')
        print(f'Lowering missing: {total["missing"]}')
        print(f'Certified parity: {total["certified"]}')
        for row in report["modules"]:
            if row["missing"]:
                print(f'{row["module"]}: {", ".join(row["missing"])}')
    if args.require_complete and (report["summary"]["missing"] or
                                  report["summary"]["certified"] != report["summary"]["members"]):
        print("SE selfhost is not API-complete and parity-certified; refusing complete status.",
              file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
