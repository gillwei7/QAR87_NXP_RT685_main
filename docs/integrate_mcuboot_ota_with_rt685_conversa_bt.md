# 整合 `evkmimxrt685_ota_mcuboot_basic` 與 `Rt685Conversa_handsfree_cm33`

## 1. 先釐清「整合」代表什麼

在 NXP / MCUXpresso 生態裡，這兩個通常是**兩個獨立映像**，不是把原始碼硬湊成同一個可執行檔：

| 專案 | 角色 | 產物 |
|------|------|------|
| `Bootloader/evkmimxrt685_ota_mcuboot_basic` | Secondary Boot Loader（MCUBoot） | 燒在 flash 最前面的 bootloader |
| `BT_Audio_with_full_audio_path/Rt685Conversa_handsfree_cm33` | 主應用（BT Audio / EdgeFast 等） | 經簽章後的 **應用 image**，由 MCUBoot 載入 |

「整合」實務上 = **同一套 flash 分區 + 同一顆 flash 上的燒錄/OTA 流程**，應用專案必須符合 MCUBoot 對 **image 起始位址、長度、簽章** 的約束。

---

## 2. 你 repo 裡兩邊目前的 flash 假設（重點）

### Bootloader（`flash_partitioning.h`）

- `BOOT_FLASH_BASE`：`0x08000000`
- Primary（active app）：`0x08040000`
- Secondary（candidate app）：`0x08240000`
- 每個 slot 大小（由兩者相減）：**`0x200000`（2 MiB）**

Bootloader 連結腳本範例（`Debug/evkmimxrt685_ota_mcuboot_basic_cm33_Debug_memory.ld`）把 bootloader 自己的 **QSPI_FLASH** 放在 **`0x08040400`** 一段區域（與應用 slot 不同層級的用途，勿與 BT 專案直接對調）。

### BT Audio 應用（`Debug/Rt685Conversa_handsfree_cm33_Debug_memory.ld`）

應用把 **`BOARD_FLASH` 從 `0x08000000` 起劃了 3 MiB**，後面再接：

- DSP bin 區（`0x08300000` 起）
- Opus/SBC 區
- LittleFS 區（約 `0x08800000` 起）

而 **`0x08240000`～`0x08440000` 這 2 MiB** 在 MCUBoot 預設裡是 **secondary slot**。  
因此：**目前 BT 專案的 flash 地圖與 OTA 範例的 slot 定義會重疊**，不能在不改分區的情況下直接接上 MCUBoot。

---

## 3. 必須解決的三件事（否則無法穩定開機 / OTA）

1. **應用 image 的起始位址**  
   必須與 MCUBoot 的 **primary slot** 一致（你這份 bootloader 預設是 **`0x08040000`** 起；實際 image 前還有 boot / image header，連結器通常會像 bootloader 專案一樣用 **`0x08040400`** 這類偏移，需與 NXP 範例對齊，勿自行猜位址）。

2. **應用 image 不能踩到 secondary slot**  
   在維持 `BOOT_FLASH_ACT_APP` / `BOOT_FLASH_CAND_APP` 不變的前提下，主應用在 primary 內的 **最大可用連續空間約 2 MiB**（再扣 header）。  
   BT 專案目前給主 flash 區 **3 MiB**，且後續區塊起點與 secondary **重疊**，一定要 **重畫 flash map**（見下節策略）。

3. **簽章與 image 格式**  
   `sblconfig.h` 內啟用了 **`CONFIG_BOOT_SIGNATURE`（RSA-2048）** 等選項；應用必須依 MCUXpresso / NXP 文件產生 **signed image**（或沿用 SDK 附的 post-build / `imgtool` 流程），否則 MCUBoot 會拒絕開機。

---

## 4. 建議的 flash 整合策略（擇一）

### 策略 A（較接近現有 OTA 範例）：縮小 / 搬移 BT 的「主 flash」區

- 把 **應用本體 + vector + boot_hdr** 全部放進 **primary slot 允許的範圍**內。  
- **DSP / Opus / LittleFS** 整段往 **secondary 之後**（例如 **`0x08440000` 之後**）搬，並同步：
  - `Debug/*_memory.ld`（或 MCUXpresso 的 Memory Configuration / LinkServer flash）
  - 任何寫死的位址（例如 `MFLASH_FILE_BASEADDR`、連結腳本中的 `ORIGIN`）
- 優點：bootloader 的 `flash_partitioning.h` **幾乎不用動**。  
- 缺點：要確認 **主應用 + 常數資料** 是否壓得進約 **2 MiB**（很常要開 `-Os`、裁功能、或把大資源外移）。

### 策略 B：改大 slot / 改 candidate 起點（進階）

- 同時修改：
  - `Bootloader/.../source/flash_partitioning.h`（`BOOT_FLASH_ACT_APP` / `BOOT_FLASH_CAND_APP`）
  - Bootloader 內 MCUBoot 相關設定（若有 `sysflash` / `flash_map` / 裝置 tree）
  - BT 專案整張 flash map  
- 優點：較有機會塞進大型應用。  
- 缺點：**所有映像與 OTA 工具參數都要一起改**，除錯成本高。

### 關於 `CONFIG_MCUBOOT_FLASH_REMAP_ENABLE`

Bootloader 開了 **flash remap**。這表示實際「執行時看到的位址」可能經硬體重對應，但 **實體 flash 上各區塊仍不能重疊**。整合時仍要以 **實體分區 + slot 定義** 為準，並對照 NXP 該範例的 **direct-xip / remap** 說明文件。

---

## 5. MCUXpresso 建議操作順序（實務）

1. 開一個 **Workspace** 同時匯入兩個專案（bootloader + BT app），方便比對 memory / define。  
2. 以 **bootloader 的 `flash_partitioning.h`** 為「唯一真相」畫出 **實體分區表**（含 header、slot、DSP、檔案系統）。  
3. 在 BT 專案：
   - 調整 **Memory Configuration**，讓 **主程式 flash** 從 **`0x08040000`（或範例要求的對齊點）** 開始，且 **不要跨入 secondary**。  
   - 重新產生 linker script 後 **全專案重編**。  
4. 依 NXP 文件加入 / 調整 **signed image** 的 post-build（常見為 `imgtool` + key）。  
5. 燒錄順序（概念）：先燒 **bootloader**，再燒 **primary slot 的 signed app**；OTA 則寫入 **secondary** 後由 MCUBoot 升級（依 swap/remap 模式而定）。

---

## 6. 建議你再補充給協作者的資訊（可加快收斂）

- 目標：**一定要 OTA**，還是只要 **MCUBoot 開機驗簽**？  
- 主應用 axf **目前大小**（`arm-none-eabi-size`）與是否 **一定得保留 3 MiB BOARD_FLASH**。  
- DSP / Opus / LittleFS 是否可 **全部搬到 `0x08440000` 之後**。  
- 是否沿用範例的 **RSA key** 與 **image 版本號** 流程。

---

## 7. 相關檔案索引（本 repo）

- Bootloader slot 定義：`Bootloader/evkmimxrt685_ota_mcuboot_basic/source/flash_partitioning.h`  
- Bootloader flash map 表：`Bootloader/evkmimxrt685_ota_mcuboot_basic/source/flash_partitioning.c`  
- Bootloader crypto / remap 總開關：`Bootloader/evkmimxrt685_ota_mcuboot_basic/source/sblconfig.h`  
- BT 應用 flash 區塊：`BT_Audio_with_full_audio_path/Rt685Conversa_handsfree_cm33/Debug/Rt685Conversa_handsfree_cm33_Debug_memory.ld`  
- BT 專案編譯定義：`BT_Audio_with_full_audio_path/Rt685Conversa_handsfree_cm33/.cproject`（含 `MFLASH_FILE_BASEADDR` 等）

---

若你希望我**直接在 BT 專案改 Memory / linker 初稿**（策略 A），請回覆：  
可接受的 **主程式最大 flash**（是否接受約 2 MiB）、以及 **DSP/Opus/LittleFS 新起始位址** 是否一律搬到 **`0x08440000` 之後**。
