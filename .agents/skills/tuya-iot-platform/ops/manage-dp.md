# Manage DPs (功能点)

Manage data points (DPs) for a product via `tuya-devplat-cli`.

See [../SKILL.md](../SKILL.md) for auth, output format, and dry-run/confirm conventions.

All commands require `--product-id <pid>`.

---

## DP ID convention

| ID range | Type | When to use |
|----------|------|-------------|
| 1 – 100 | Standard DP | Platform-defined, available via `dp-standard-catalog`. **Use first** if it covers the required function. |
| 101+ | Custom DP | Define yourself when no standard DP satisfies the requirement. |

**Decision rule:**
1. Check `dp-standard-catalog` — if a standard DP (1–100) covers the function, use it.
2. Only create a custom DP (101+) when standard DPs cannot satisfy the requirement.

> **`qt` (其他) category:** The standard catalog is empty — no predefined standard DPs exist.
> All DPs are custom and can start from ID 1, with no range restrictions.
> This is why `qt` is the preferred category: it gives full freedom over DP definition
> without needing to check or conform to any standard DP catalog.

---

## Standard path — helper script

`scripts/manage_dp.py` wraps all operations into a single cross-platform command:

```bash
# List attached DPs
python scripts/manage_dp.py list --product-id <pid>

# Browse standard catalog (all DPs, mark attached/not)
python scripts/manage_dp.py catalog --product-id <pid>

# Show only DPs not yet attached
python scripts/manage_dp.py catalog --product-id <pid> --available

# Add DPs (dry-run → confirm automatically)
python scripts/manage_dp.py add --product-id <pid> --dp-ids 20,21,22

# Remove one DP (dry-run → confirm automatically)
python scripts/manage_dp.py remove --product-id <pid> --dp-id 20

# Validate DP configuration
python scripts/manage_dp.py validate --product-id <pid>
```

Use the manual steps below only when you need to inspect intermediate results.

---

## Operations overview

| Goal | Command | Mutating |
|------|---------|----------|
| List attached DPs | `product dp-list` | No |
| Browse standard catalog (all DPs for category) | `product dp-standard-catalog` | No |
| Get full DP schema (incl. cloud functions) | `product dp-schema` | No |
| Validate current DP config | `product dp-valid` | No |
| Add standard DPs | `product dp-add-standard` | Yes |
| Remove one standard DP | `product dp-remove-standard` | Yes |

---

## Read operations

### List attached DPs

```bash
tuya-devplat-cli product dp-list --product-id <pid> --format json
```

Returns the DPs currently attached to the product. Key fields per entry:
`id`, `code`, `name`, `required`, `selected`, `standarded`.

> Note: `dp-list` does NOT return `mode` or `type`. Use `dp-schema` if you need full DP property definitions.

### Browse standard catalog

```bash
tuya-devplat-cli product dp-standard-catalog --product-id <pid> --format json
```

Returns `data.standardDps[]` — all standard DPs for the product's category.
`selected: true` means the DP is already attached; `selected: false` means it is available to add.

Use this to find DP `id` values before calling `dp-add-standard`.

### Get full DP schema

```bash
tuya-devplat-cli product dp-schema --product-id <pid> --format json
```

Returns the full schema including cloud function (BIC) definitions.
Use when you need property details (range, scale, step) or cloud function config.

### Validate DPs

```bash
tuya-devplat-cli product dp-valid --product-id <pid> --format json
```

Returns the full DP definitions (including `mode`, `type`, `property`) for DPs attached to the product.
Use this to inspect the current DP configuration in detail — it is not a pass/fail validation report.

---

## Write operations

### Add standard DPs

DP IDs come from `dp-standard-catalog` → `data.standardDps[].id`.

```bash
# dry-run
tuya-devplat-cli product dp-add-standard \
  --product-id <pid> \
  --self-dps '[20, 21, 22]' \
  --dry-run --format json

# confirm
tuya-devplat-cli product dp-add-standard \
  --product-id <pid> \
  --self-dps '[20, 21, 22]' \
  --confirm <confirm_token> --format json
```

- `--self-dps`: JSON array of DP IDs to attach (required)
- `--another-dps`: JSON array of DP IDs from another category (optional)

### Remove one standard DP

```bash
# dry-run
tuya-devplat-cli product dp-remove-standard \
  --product-id <pid> \
  --dp-id <id> \
  --dry-run --format json

# confirm
tuya-devplat-cli product dp-remove-standard \
  --product-id <pid> \
  --dp-id <id> \
  --confirm <confirm_token> --format json
```

Only one DP can be removed per call.

---

## Typical workflow

```bash
# 1. See what is already attached
tuya-devplat-cli product dp-list --product-id <pid> --format json

# 2. Browse available DPs (selected=false = not yet attached)
tuya-devplat-cli product dp-standard-catalog --product-id <pid> --format json

# 3. Add DPs by ID (use id values from step 2; for qt, start from 1)
tuya-devplat-cli product dp-add-standard \
  --product-id <pid> --self-dps '[1,2,3]' --dry-run --format json
tuya-devplat-cli product dp-add-standard \
  --product-id <pid> --self-dps '[1,2,3]' --confirm <token> --format json

# 4. Inspect full DP definitions
tuya-devplat-cli product dp-valid --product-id <pid> --format json
```

---

## Troubleshooting

| Error | Fix |
|-------|-----|
| `dp-list` returns empty array | Product has no DPs attached yet — use `dp-standard-catalog` to find available IDs |
| `confirm_token` rejected (`INVALID_CONFIRMATION`) | Token is one-time — re-run `--dry-run` to get a new one |
| DP ID not found in catalog | Use `dp-standard-catalog` to confirm the ID exists for this product's category; for `qt`, any ID from 1 up is valid |
