# Product Management

Search, inspect, and create products on the Tuya Developer Platform.

See [../SKILL.md](../SKILL.md) for auth, output format, and dry-run/confirm conventions.

---

## Standard path — helper script

`scripts/product.py` covers all three operations:

> If `tuya-devplat-cli` is not in PATH, set `TUYA_DEVPLAT_CLI` to the binary path:
> ```bash
> export TUYA_DEVPLAT_CLI=".tuyaopen/ide/bin/tuya-devplat-cli"
> ```
> The script also auto-detects the binary at `.tuyaopen/ide/bin/tuya-devplat-cli` by
> searching upward from the current working directory.

```bash
# List all products (default max 20)
python scripts/product.py list

# Search by keyword
python scripts/product.py list --keyword "test"

# Get product details by PID
python scripts/product.py detail --product-id <pid>

# Create a product (default: wf_ble_qt, solution 10019526)
python scripts/product.py create --name "my-product"

# Create with a specific category/solution
python scripts/product.py create --name "my-socket" --category wf_ble_cz --solution-id 134001
```

---

## Manual path

### Search / list products

```bash
tuya-devplat-cli product list --format json --max-items 20
tuya-devplat-cli product list --keyword "<keyword>" --format json --max-items 20
```

Results are in `data.datas[]`. Key fields: `id`, `name`, `categoryName`, `developStatus`.

Use `--fields` to reduce output:

```bash
tuya-devplat-cli product list --format json \
  --fields id,name,categoryName,developStatus --max-items 20
```

### Get product details

```bash
tuya-devplat-cli product detail --id <pid> \
  --fields id,name,categoryName,categoryCode,communicationCodes,developStatus \
  --format json
```

### Create a product

#### Step 1 — Find the category code

Skip if using a known category from the reference table in SKILL.md.

```bash
tuya-devplat-cli product category-tree --keyword "<keyword>" --format json
```

Find the matching node; its **`code` field** is the category code.

#### Step 2 — Find the solution ID

Run `custom-list` with the raw category code:

```bash
tuya-devplat-cli product custom-list --category-code <code> --format json
```

**Case A — `solutionModuleVOS` present** (e.g., `cz`, `dj`):
Pick any entry where `capabilityCode == "wifi_bluetooth"`. Its **`code` field** is the solution ID.

**Case B — `solutionModuleVOS` absent** (e.g., `qt`):
Take `solutionGroupVOS[0].solutionGroupId`, then:

```bash
tuya-devplat-cli product communication-list \
  --solution-group-id <solutionGroupId> --format json
```

In `data[0].data[]`, find `upstream.name == "WiFi-蓝牙"`.
Use its **`solutionId`** and **`code`** (= frontend category code).

#### Step 3 — Create (dry-run → confirm)

```bash
# dry-run
tuya-devplat-cli product create-common \
  --name "<name>" --category <frontend_category> --solution-id <solution_id> \
  --dry-run --format json

# confirm
tuya-devplat-cli product create-common \
  --name "<name>" --category <frontend_category> --solution-id <solution_id> \
  --confirm <confirm_token> --format json
```

`data.id` in the confirm response is the **PID**.

#### Step 4 — Verify

```bash
tuya-devplat-cli product detail --id <pid> \
  --fields id,name,categoryName,categoryCode,communicationCodes,developStatus \
  --format json
```

Confirm `communicationCodes` contains both `"wifi"` and `"bluetooth"`.

---

## Troubleshooting

| Error | Fix |
|-------|-----|
| `CATEGORY_NOT_EXIST` | Pass English code (e.g., `cz`), not Chinese name |
| `solutionModuleVOS` absent | Use `communication-list` fallback (Case B) |
| `confirm_token` rejected (`INVALID_CONFIRMATION`) | Re-run `--dry-run` to get a new token |
| `API_OR_API_VERSION_WRONG` | Check auth: `tuya-devplat-cli auth status` |
