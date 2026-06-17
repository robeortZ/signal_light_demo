import React, { useState, useRef, useEffect } from 'react';
import { View, Text, Input } from '@ray-js/ray';
import { useProps, useActions } from '@ray-js/panel-sdk';
import styles from './index.module.less';

interface DpEntry {
  code: string;
  name?: string;
  mode?: string;
}

const HEX_RE = /^[0-9a-fA-F]*$/;

function formatHexPreview(s: string): string {
  // Insert a space every 2 hex chars for readability — purely cosmetic.
  return s.replace(/(.{2})/g, '$1 ').trim();
}

export default function RawDp({ dp }: { dp: DpEntry }) {
  const props = useProps((p: any) => p) as Record<string, any>;
  const actions = useActions() as Record<string, { set: (v: any) => Promise<boolean> }>;
  const [hexValue, setHexValue] = useState('');
  const [error, setError] = useState('');
  const [sentAt, setSentAt] = useState<number>(0);
  const isReadOnly = (dp as any).mode === 'ro';

  // useProps mirrors every dpChange event — including the echo of our own
  // publishes (host broadcasts back with source='shim'). Without filtering
  // RX would display the bytes we just sent. Track our last-sent value in
  // a ref and only commit to rxValue when an incoming change differs from
  // it. The first change after a send that matches `lastSentRef` is the
  // echo and gets dropped.
  const lastSentRef = useRef<string>('');
  const [rxValue, setRxValue] = useState<string>('');
  const lastReceived = props?.[dp.code];
  const lastReceivedStr = typeof lastReceived === 'string' ? lastReceived : '';
  useEffect(() => {
    if (!lastReceivedStr) return;
    if (lastReceivedStr === lastSentRef.current) {
      // Consume the echo — clear the marker so any subsequent identical
      // payload from the device side does land in RX.
      lastSentRef.current = '';
      return;
    }
    setRxValue(lastReceivedStr);
  }, [lastReceivedStr]);

  const handleSend = () => {
    const val = hexValue.replace(/\s/g, '');
    if (!val) { setError('Empty payload'); return; }
    if (!HEX_RE.test(val)) { setError('Invalid hex string'); return; }
    if (val.length % 2 !== 0) { setError('Hex length must be even'); return; }
    setError('');
    lastSentRef.current = val;
    actions?.[dp.code]?.set?.(val).then(() => {
      setSentAt(Date.now());
    }).catch((err: any) => {
      // Send failed — clear the echo guard so the next legit RX can land.
      if (lastSentRef.current === val) lastSentRef.current = '';
      setError(String(err?.message ?? 'Send failed'));
    });
  };

  const justSent = sentAt > 0 && Date.now() - sentAt < 1500;
  const cleanHex = hexValue.replace(/\s/g, '');
  const byteCount = Math.floor(cleanHex.length / 2);

  return (
    <View className={styles.rawCard}>
      <View className={styles.rawHeader}>
        <View className={styles.rawIcon}>
          <Text className={styles.rawIconGlyph}>⬡</Text>
        </View>
        <View className={styles.rawTitleWrap}>
          <Text className={styles.rawName}>{(dp as any).name || dp.code}</Text>
          <Text className={styles.rawCode}>{dp.code}</Text>
        </View>
        <View className={styles.rawByteBadge}>
          <Text className={styles.rawByteCount}>{byteCount}</Text>
          <Text className={styles.rawByteLabel}>B</Text>
        </View>
      </View>

      {!!rxValue && (
        <View className={styles.rawRxRow}>
          <Text className={styles.rawRxLabel}>RX</Text>
          <Text className={styles.rawRxValue}>{formatHexPreview(rxValue) || '—'}</Text>
        </View>
      )}

      {!isReadOnly && (
        <>
          <View className={styles.rawInputRow}>
            <Input
              className={styles.rawInput}
              value={hexValue}
              placeholder="Enter hex bytes, e.g. 1A 2B 3C"
              onInput={(e: any) => {
                const v = (e.detail?.value ?? '').replace(/\s/g, '');
                setHexValue(v);
                if (error) setError('');
              }}
            />
          </View>
          <View
            className={`${styles.rawSendBtn} ${justSent ? styles.rawSendBtnSuccess : ''}`}
            onClick={handleSend}
          >
            <Text className={styles.rawSendBtnIcon}>{justSent ? '✓' : '↗'}</Text>
            <Text className={styles.rawSendBtnText}>{justSent ? 'Sent' : 'Send'}</Text>
          </View>
        </>
      )}
      {!!error && (
        <View className={styles.rawErrorRow}>
          <Text className={styles.rawErrorIcon}>⚠</Text>
          <Text className={styles.rawError}>{error}</Text>
        </View>
      )}
    </View>
  );
}
