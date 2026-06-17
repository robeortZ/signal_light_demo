import React from 'react';
import { View, Text } from '@ray-js/ray';
import { useProps, useActions } from '@ray-js/panel-sdk';
import styles from './index.module.less';

interface DpEntry {
  code: string;
  name?: string;
  mode?: string;
  property?: {
    type?: string;
    max?: number;
    label?: string[];
  };
}

export default function BitmapDp({ dp }: { dp: DpEntry }) {
  const props = useProps((p: any) => p) as Record<string, any>;
  const actions = useActions() as Record<string, { set: (v: any) => Promise<boolean> }>;
  const property: any = dp.property || {};
  const max = typeof property.max === 'number' ? property.max : 255;
  const bitCount = Math.max(1, Math.ceil(Math.log2(max + 1)));
  const labels: string[] = Array.isArray(property.label) ? property.label : [];
  const rawValue = typeof props?.[dp.code] === 'number' ? (props[dp.code] as number) : 0;
  const isReadOnly = (dp as any).mode === 'ro';

  const toggleBit = (bitIdx: number, checked: boolean) => {
    if (isReadOnly) return;
    const newVal = checked
      ? rawValue | (1 << bitIdx)
      : rawValue & ~(1 << bitIdx);
    actions?.[dp.code]?.set?.(newVal);
  };

  const bits = Array.from({ length: bitCount }, (_, i) => ({
    idx: i,
    label: labels[i] || `bit${i}`,
    checked: !!(rawValue & (1 << i)),
  }));
  const activeCount = bits.filter((b) => b.checked).length;
  const hex = '0x' + rawValue.toString(16).toUpperCase().padStart(2, '0');
  const bin = rawValue.toString(2).padStart(bitCount, '0');

  return (
    <View className={styles.bitmapCard}>
      <View className={styles.bitmapHeader}>
        <View className={styles.bitmapIcon}>
          <Text className={styles.bitmapIconGlyph}>⚑</Text>
        </View>
        <View className={styles.bitmapTitleWrap}>
          <Text className={styles.bitmapName}>{(dp as any).name || dp.code}</Text>
          <Text className={styles.bitmapCode}>{dp.code}</Text>
        </View>
        <View className={`${styles.bitmapState} ${activeCount > 0 ? styles.bitmapStateActive : ''}`}>
          <Text className={styles.bitmapStateCount}>{activeCount}</Text>
          <Text className={styles.bitmapStateLabel}>/ {bitCount}</Text>
        </View>
      </View>

      <View className={styles.bitmapValueRow}>
        <View className={styles.bitmapValueChip}>
          <Text className={styles.bitmapValueLabel}>HEX</Text>
          <Text className={styles.bitmapValueText}>{hex}</Text>
        </View>
        <View className={styles.bitmapValueChip}>
          <Text className={styles.bitmapValueLabel}>BIN</Text>
          <Text className={styles.bitmapValueText}>{bin}</Text>
        </View>
      </View>

      <View className={styles.bitmapBits}>
        {bits.map((bit) => (
          <View
            key={bit.idx}
            className={`${styles.bitmapBit} ${bit.checked ? styles.bitmapBitActive : ''} ${isReadOnly ? styles.bitmapBitReadOnly : ''}`}
            onClick={() => toggleBit(bit.idx, !bit.checked)}
          >
            <Text className={styles.bitmapBitIdx}>{bit.idx}</Text>
            <Text className={styles.bitmapBitLabel}>{bit.label}</Text>
          </View>
        ))}
      </View>
    </View>
  );
}
