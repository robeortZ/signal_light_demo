# Board Hardware Catalog: tuya-t5ai-core
Platform: t5ai

## audio — Audio
Interface: INTERNAL
Pins: spk_en=GPIO39(high)
Note: AEC enabled by default; designed for far-field voice interaction.

## led — Indicator LED
Interface: GPIO
Config: count=1
Pins: led=GPIO9(high)
Note: Active high.

## button — User Button
Interface: GPIO
Config: count=1
Pins: btn=GPIO29(low)
Note: Active low.
