import React, { useEffect, useState } from 'react';
import { View, Text, Button } from '@ray-js/ray';
import { checkOta, OtaInfo } from '@/utils/ota';
import styles from './index.module.less';

declare const ty: any;

type OtaState = 'idle' | 'available' | 'upgrading' | 'done' | 'error';

export default function OtaCard({ onUpgradingChange }: { onUpgradingChange?: (v: boolean) => void }) {
  const [state, setState] = useState<OtaState>('idle');
  const [otaInfo, setOtaInfo] = useState<OtaInfo | null>(null);
  const [progress, setProgress] = useState(0);
  const [errMsg, setErrMsg] = useState('');

  useEffect(() => {
    checkOta().then(info => {
      if (info) { setOtaInfo(info); setState('available'); }
    });
  }, []);

  const handleUpgrade = async () => {
    if (!otaInfo) return;
    setState('upgrading');
    onUpgradingChange?.(true);
    setProgress(0);
    try {
      await otaInfo.upgrade(pct => setProgress(pct));
      setState('done');
      onUpgradingChange?.(false);
      if (typeof ty !== 'undefined' && ty.showToast) {
        ty.showToast({ title: '升级成功', icon: 'success' });
      }
    } catch (e: any) {
      setState('error');
      onUpgradingChange?.(false);
      setErrMsg(String(e?.message ?? '升级失败'));
      if (typeof ty !== 'undefined' && ty.showToast) {
        ty.showToast({ title: String(e?.message ?? '升级失败'), icon: 'error' });
      }
    }
  };

  if (state === 'idle' || state === 'done') return null;

  return (
    <View className={styles.otaCard}>
      <Text className={styles.otaTitle}>固件升级可用</Text>
      <Text className={styles.otaMeta}>
        版本 {otaInfo?.targetVersion ?? '–'}
        {otaInfo?.firmwareSize ? `  ·  ${(otaInfo.firmwareSize / 1024).toFixed(0)} KB` : ''}
      </Text>
      {state === 'upgrading' && (
        <View className={styles.otaProgressBar}>
          <View className={styles.otaProgressFill} style={{ width: `${progress}%` }} />
        </View>
      )}
      {state === 'error' && (
        <Text className={styles.otaMeta}>{errMsg}</Text>
      )}
      <Button
        className={`${styles.otaBtn} ${state === 'upgrading' ? styles.otaBtnDisabled : ''}`}
        disabled={state === 'upgrading'}
        onClick={handleUpgrade}
      >
        {state === 'upgrading' ? `升级中 ${progress}%` : state === 'error' ? '重试' : '立即升级'}
      </Button>
    </View>
  );
}
