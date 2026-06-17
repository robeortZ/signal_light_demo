import React from 'react';
import { View, Text, Switch, Slider, Input } from '@ray-js/ray';
import { useProps, useActions } from '@ray-js/panel-sdk';
import { lampSchema } from '@/devices/schema';
import { isOfflinePreview } from '@/devices';
import styles from './index.module.less';
import BitmapDp from './components/BitmapDp';
import RawDp from './components/RawDp';
import OtaCard from './components/OtaCard';

type SchemaItem = (typeof lampSchema)[number];

// In offline preview the user has no real device to drive — every DP must
// be writable locally so they can inspect the panel's response curves.
// Production runs honour the cloud schema's `mode` exactly.
const PREVIEW_MODE = isOfflinePreview();
function isWritable(dp: SchemaItem): boolean {
  if (PREVIEW_MODE) return true;
  return (dp as any).mode !== 'ro';
}

// Heuristic icon mapping based on DP code keywords. Falls back to a
// neutral glyph for unrecognised codes — keeps the layout consistent
// across arbitrary cloud-synced schemas.
function iconFor(code: string): string {
  const c = code.toLowerCase();
  if (/^switch/.test(c) || /power/.test(c)) return '⏻';
  if (/battery/.test(c)) return '🔋';
  if (/charge/.test(c)) return '⚡';
  if (/(volume|sound)/.test(c)) return '🔊';
  if (/bright/.test(c)) return '☀';
  if (/color_temp|colour_temp/.test(c)) return '🎨';
  if (/temp/.test(c)) return '🌡';
  if (/humid/.test(c)) return '💧';
  if (/(work_mode|^mode$)/.test(c)) return '⚙';
  if (/(timer|countdown)/.test(c)) return '⏱';
  if (/(feed|food|grain)/.test(c)) return '🥣';
  if (/light/.test(c)) return '💡';
  if (/lock/.test(c)) return '🔒';
  if (/(door|window)/.test(c)) return '🚪';
  return '◉';
}

function isPrimarySwitch(dp: SchemaItem): boolean {
  const code = String((dp as any).code || '').toLowerCase();
  const t = ((dp as any).property || {}).type;
  return t === 'bool' && (code === 'switch' || code === 'switch_led' || /^switch_?$/.test(code) || code === 'power');
}

// Slider's onChange may pass either a raw number (Tuya ty-slider) or a
// wrapped event with detail.value (basic Ray slider). Accept both.
function readSliderValue(e: any): number | null {
  const v = typeof e === 'number' ? e : Number(e?.detail?.value);
  return Number.isFinite(v) ? v : null;
}

function HeroCard({ dp }: { dp: SchemaItem }) {
  const props = useProps((p: any) => p) as Record<string, any>;
  const actions = useActions() as Record<string, { set: (v: any) => Promise<boolean> }>;
  const value = !!props?.[dp.code];
  const readOnly = !isWritable(dp);
  return (
    <View className={`${styles.hero} ${value ? styles.heroOn : styles.heroOff}`}>
      <View className={styles.heroHalo} />
      <View className={styles.heroHeader}>
        <View className={styles.heroBrand}>
          <Text className={styles.title}>Smart Panel</Text>
          <Text className={styles.subtitle}>TuyaOpen IDE</Text>
        </View>
        <View className={`${styles.statusPill} ${PREVIEW_MODE ? styles.statusPillPreview : ''}`}>
          <View className={styles.dot} />
          <Text className={styles.statusText}>{PREVIEW_MODE ? 'Preview' : 'Online'}</Text>
        </View>
      </View>
      <View className={styles.heroIcon}>
        <Text className={styles.heroIconGlyph}>⏻</Text>
      </View>
      <Text className={styles.heroName}>{(dp as any).name || dp.code}</Text>
      <Text className={styles.heroState}>{value ? 'ON' : 'OFF'}</Text>
      <View className={styles.heroToggle}>
        <Switch
          checked={value}
          disabled={readOnly}
          onChange={(e: any) => {
            if (readOnly) return;
            actions?.[dp.code]?.set?.(!!e?.detail?.value);
          }}
        />
      </View>
    </View>
  );
}

function BoolRow({ dp }: { dp: SchemaItem }) {
  const props = useProps((p: any) => p) as Record<string, any>;
  const actions = useActions() as Record<string, { set: (v: any) => Promise<boolean> }>;
  const value = !!props?.[dp.code];
  const readOnly = !isWritable(dp);
  return (
    <View className={styles.card}>
      <View className={styles.cardIcon}>
        <Text className={styles.cardIconGlyph}>{iconFor((dp as any).code)}</Text>
      </View>
      <View className={styles.cardBody}>
        <Text className={styles.cardName}>{(dp as any).name || dp.code}</Text>
        <Text className={styles.cardMeta}>{(dp as any).code}</Text>
      </View>
      <Switch
        checked={value}
        disabled={readOnly}
        onChange={(e: any) => {
          if (readOnly) return;
          actions?.[dp.code]?.set?.(!!e?.detail?.value);
        }}
      />
    </View>
  );
}

function ReadonlyValueRow({ dp, num, pct, unit, isBattery }: {
  dp: SchemaItem; num: number; pct: number; unit: string; isBattery: boolean;
}) {
  return (
    <View className={styles.sliderCard}>
      <View className={styles.sliderHeader}>
        <View className={styles.cardIcon}>
          <Text className={styles.cardIconGlyph}>{iconFor((dp as any).code)}</Text>
        </View>
        <Text className={styles.sliderName}>{(dp as any).name || dp.code}</Text>
        <View className={styles.sliderValueWrap}>
          <Text className={`${styles.sliderValueBig} ${isBattery ? styles.sliderValueBattery : ''}`}>{num}</Text>
          {!!unit && <Text className={`${styles.sliderValueUnit} ${isBattery ? styles.sliderValueBatteryUnit : ''}`}>{unit}</Text>}
        </View>
      </View>
      <View className={styles.barLarge}>
        <View
          className={`${styles.barLargeFill} ${isBattery ? styles.barBattery : ''}`}
          style={{ width: `${pct}%` }}
        />
      </View>
      <View className={styles.sliderRangeFlat}>
        <Text className={styles.sliderRangeLabel}>0{unit}</Text>
        <Text className={styles.sliderRangeLabel}>{unit ? `100${unit}` : '100'}</Text>
      </View>
    </View>
  );
}

function InteractiveSliderCard({ dp, num, min, max, step, unit }: {
  dp: SchemaItem; num: number; min: number; max: number; step: number; unit: string;
}) {
  const actions = useActions() as Record<string, { set: (v: any) => Promise<boolean> }>;
  const set = (val: number) => {
    const clamped = Math.max(min, Math.min(max, val));
    actions?.[dp.code]?.set?.(clamped);
  };
  return (
    <View className={styles.sliderCard}>
      <View className={styles.sliderHeader}>
        <View className={styles.cardIcon}>
          <Text className={styles.cardIconGlyph}>{iconFor((dp as any).code)}</Text>
        </View>
        <Text className={styles.sliderName}>{(dp as any).name || dp.code}</Text>
        <View className={styles.sliderValueWrap}>
          <Text className={styles.sliderValueBig}>{num}</Text>
          {!!unit && <Text className={styles.sliderValueUnit}>{unit}</Text>}
        </View>
      </View>
      <View className={styles.sliderRow}>
        <View className={styles.adjBtn} onClick={() => set(num - step)}>
          <Text className={styles.adjGlyph}>−</Text>
        </View>
        <View className={styles.sliderTrack}>
          <Slider
            min={min}
            max={max}
            step={step}
            value={num}
            maxTrackColor="#e2e8f0"
            minTrackColor="linear-gradient(90deg, #6366f1, #22d3ee)"
            maxTrackHeight="20px"
            minTrackHeight="20px"
            thumbColor="#ffffff"
            thumbWidth="36px"
            thumbHeight="36px"
            thumbBoxShadowStyle="0px 4px 12px rgba(99,102,241,0.40)"
            onChange={(e: any) => {
              const v = readSliderValue(e);
              if (v !== null) set(v);
            }}
          />
        </View>
        <View className={styles.adjBtn} onClick={() => set(num + step)}>
          <Text className={styles.adjGlyph}>+</Text>
        </View>
      </View>
      <View className={styles.sliderRange}>
        <Text className={styles.sliderRangeLabel}>{min}{unit}</Text>
        <Text className={styles.sliderRangeLabel}>{max}{unit}</Text>
      </View>
    </View>
  );
}

function ValueRow({ dp }: { dp: SchemaItem }) {
  const props = useProps((p: any) => p) as Record<string, any>;
  const property: any = (dp as any).property || {};
  const min = typeof property.min === 'number' ? property.min : 0;
  const max = typeof property.max === 'number' ? property.max : 100;
  const step = typeof property.step === 'number' ? property.step : 1;
  const v = props?.[dp.code];
  const num = typeof v === 'number' ? v : min;
  const pct = max > min ? Math.max(0, Math.min(100, ((num - min) / (max - min)) * 100)) : 0;
  const readOnly = !isWritable(dp);
  const isBattery = /battery/.test(String(dp.code).toLowerCase());
  const unit = property.unit || (isBattery ? '%' : '');

  return readOnly
    ? <ReadonlyValueRow dp={dp} num={num} pct={pct} unit={unit} isBattery={isBattery} />
    : <InteractiveSliderCard dp={dp} num={num} min={min} max={max} step={step} unit={unit} />;
}

function EnumRow({ dp }: { dp: SchemaItem }) {
  const props = useProps((p: any) => p) as Record<string, any>;
  const actions = useActions() as Record<string, { set: (v: any) => Promise<boolean> }>;
  const property: any = (dp as any).property || {};
  const range: string[] = Array.isArray(property.range) ? property.range : [];
  const value = props?.[dp.code];
  const readOnly = !isWritable(dp);
  return (
    <View className={styles.card}>
      <View className={styles.cardIcon}>
        <Text className={styles.cardIconGlyph}>{iconFor((dp as any).code)}</Text>
      </View>
      <View className={styles.cardBody}>
        <Text className={styles.cardName}>{(dp as any).name || dp.code}</Text>
        <View className={styles.pillRow}>
          {range.map((opt) => {
            const active = value === opt;
            return (
              <View
                key={opt}
                className={`${styles.pill} ${active ? styles.pillActive : ''}`}
                onClick={() => {
                  if (readOnly) return;
                  actions?.[dp.code]?.set?.(opt);
                }}
              >
                <Text className={`${styles.pillText} ${active ? styles.pillTextActive : ''}`}>{opt}</Text>
              </View>
            );
          })}
        </View>
      </View>
    </View>
  );
}

function StringRow({ dp }: { dp: SchemaItem }) {
  const props = useProps((p: any) => p) as Record<string, any>;
  const actions = useActions() as Record<string, { set: (v: any) => Promise<boolean> }>;
  const value = props?.[dp.code];
  const valueStr = value === undefined || value === null ? '' : String(value);
  const readOnly = !isWritable(dp);
  const property: any = (dp as any).property || {};
  const maxlen = typeof property.maxlen === 'number' ? property.maxlen : 0;
  const [draft, setDraft] = React.useState('');
  const [sentAt, setSentAt] = React.useState(0);
  const [error, setError] = React.useState('');
  React.useEffect(() => { setDraft(valueStr); }, [valueStr]);
  const justSent = sentAt > 0 && Date.now() - sentAt < 1500;

  const handleSend = () => {
    if (readOnly) return;
    if (maxlen > 0 && draft.length > maxlen) {
      setError(`Too long (max ${maxlen})`);
      return;
    }
    setError('');
    actions?.[dp.code]?.set?.(draft).then(() => setSentAt(Date.now())).catch((err: any) => {
      setError(String(err?.message ?? 'Send failed'));
    });
  };

  return (
    <View className={styles.card}>
      <View className={styles.cardIcon}>
        <Text className={styles.cardIconGlyph}>{iconFor((dp as any).code)}</Text>
      </View>
      <View className={styles.cardBody}>
        <View className={styles.stringHeaderRow}>
          <Text className={styles.cardName}>{(dp as any).name || dp.code}</Text>
          {maxlen > 0 && (
            <Text className={styles.stringLen}>{draft.length}/{maxlen}</Text>
          )}
        </View>
        {readOnly ? (
          <Text className={styles.cardRaw}>{valueStr || '-'}</Text>
        ) : (
          <>
            <View className={styles.stringInputRow}>
              <Input
                className={styles.stringInput}
                value={draft}
                placeholder="Enter text…"
                maxlength={maxlen > 0 ? maxlen : -1}
                onInput={(e: any) => {
                  setDraft(e.detail?.value ?? '');
                  if (error) setError('');
                }}
              />
              <View
                className={`${styles.stringSendBtn} ${justSent ? styles.stringSendBtnSuccess : ''}`}
                onClick={handleSend}
              >
                <Text className={styles.stringSendBtnText}>{justSent ? '✓' : '↗'}</Text>
              </View>
            </View>
            {!!error && <Text className={styles.stringError}>{error}</Text>}
          </>
        )}
      </View>
    </View>
  );
}

function DpRow({ dp }: { dp: SchemaItem }) {
  const t = ((dp as any).property || {}).type;
  if (t === 'bool')   return <BoolRow   dp={dp} />;
  if (t === 'value')  return <ValueRow  dp={dp} />;
  if (t === 'enum')   return <EnumRow   dp={dp} />;
  if (t === 'bitmap') return <BitmapDp  dp={dp as any} />;
  if (t === 'raw')    return <RawDp     dp={dp as any} />;
  return <StringRow dp={dp} />;
}

export default function Home() {
  const list = lampSchema as readonly SchemaItem[];
  const primary = list.find(isPrimarySwitch);
  const rest = primary ? list.filter((d) => d !== primary) : list;
  const [isUpgrading, setIsUpgrading] = React.useState(false);

  return (
    <View className={styles.container}>
      {primary
        ? <HeroCard dp={primary} />
        : (
          <View className={`${styles.hero} ${styles.heroOff}`}>
            <View className={styles.heroHalo} />
            <View className={styles.heroHeader}>
              <View className={styles.heroBrand}>
                <Text className={styles.title}>Smart Panel</Text>
                <Text className={styles.subtitle}>TuyaOpen IDE</Text>
              </View>
              <View className={`${styles.statusPill} ${PREVIEW_MODE ? styles.statusPillPreview : ''}`}>
                <View className={styles.dot} />
                <Text className={styles.statusText}>{PREVIEW_MODE ? 'Preview' : 'Online'}</Text>
              </View>
            </View>
          </View>
        )}

      {list.length === 0 ? (
        <View className={styles.emptyCard}>
          <Text className={styles.emptyIcon}>🔌</Text>
          <Text className={styles.emptyTitle}>No DPs synced yet</Text>
          <Text className={styles.emptyHint}>
            Run "Sync DPs from Cloud" in the TuyaOpen IDE MiniApp panel to
            generate src/devices/schema.ts.
          </Text>
        </View>
      ) : rest.length > 0 ? (
        <View style={{ pointerEvents: isUpgrading ? 'none' : undefined, opacity: isUpgrading ? 0.5 : 1 } as any}>
          <View className={styles.sectionHeader}>
            <View className={styles.sectionAccent} />
            <Text className={styles.sectionTitle}>Functions</Text>
          </View>
          {rest.map((dp) => <DpRow key={dp.code} dp={dp} />)}
        </View>
      ) : null}
      <OtaCard onUpgradingChange={setIsUpgrading} />
    </View>
  );
}
