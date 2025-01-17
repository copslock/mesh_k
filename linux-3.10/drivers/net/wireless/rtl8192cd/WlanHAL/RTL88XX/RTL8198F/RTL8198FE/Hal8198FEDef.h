#ifndef __HAL8198FE_DEF_H__
#define __HAL8198FE_DEF_H__

#if (BEAMFORMING_SUPPORT == 1)

VOID
Beamforming_NDPARate_8198F(
	struct rtl8192cd_priv *priv,
	BOOLEAN		Mode,
	u1Byte		BW,
	u1Byte		Rate
);

VOID
SetBeamformStatus8198F(
	struct rtl8192cd_priv *priv,
	 u1Byte				Idx
);

VOID
SetBeamformLeave8198F(
	struct rtl8192cd_priv *priv,
	u1Byte				Idx
);

VOID
SetBeamformEnter8198F(
	struct rtl8192cd_priv *priv,
	u1Byte				BFerBFeeIdx
);

VOID
C2HTxBeamformingHandler_8198F(
	struct rtl8192cd_priv *priv,
		pu1Byte			CmdBuf,
		u1Byte			CmdLen
);
#endif

#endif  //__HAL8198FE_DEF_H__

