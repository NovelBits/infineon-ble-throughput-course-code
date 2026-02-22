# Bluetooth LE Data Throughput Course — Lab Code

Course code for the **Bluetooth LE Data Throughput** course using Infineon CYW920829M2EVK-02 evaluation kits.

Each lab folder contains complete ModusToolbox projects that you can import directly into the IDE. Labs with a `starter/` and `solution/` split have TODO comments in the starter code to guide you through the exercise.

## Hardware Required

- 2x Infineon CYW920829M2EVK-02 Evaluation Kits
- 1x CY8CKIT-028-SENSE IoT Sense Expansion Shield (optional, for OLED display)
- micro-USB cables for both boards

## Getting Started

### 1. Clone this repository

```bash
git clone https://github.com/NovelBits/infineon-ble-throughput-course-code.git
```

### 2. Import a lab into ModusToolbox

1. Open **ModusToolbox Eclipse IDE**
2. Go to **File > Import > ModusToolbox > ModusToolbox Application Import**
3. Browse to the lab folder you want to work on (e.g., `lab1_baseline/central/`)
4. The IDE will resolve BSP and library dependencies automatically

### 3. Build and flash

1. Select the project in the Project Explorer
2. Click **Build** (hammer icon)
3. Connect the target board via micro-USB
4. Click **Program** (green play icon with chip)

## Lab Map

| Lab | Folder | Description | What You Do |
|-----|--------|-------------|-------------|
| **Lab 1** | `lab1_baseline/` | Environment setup and baseline measurement | Import, build, flash, verify connection. All feature flags OFF. |
| **Lab 2** | `lab2_phy_update/` | PHY configuration — switch to 2M PHY | **Starter:** Enable PHY flag and select 2M PHY in `app_config.h` (2 TODOs per board). **Solution:** Working 2M PHY configuration. |

## Lab Structure

### lab1_baseline/

No starter/solution split — this is the unmodified baseline. Both projects build and run with all feature flags disabled (1M PHY, 27-byte DLE, default MTU).

```
lab1_baseline/
├── central/          # Central (initiator) — imports as a ModusToolbox project
└── peripheral/       # Peripheral (advertiser) — imports as a ModusToolbox project
```

### lab2_phy_update/

Students enable the PHY update feature flag and select the 2M PHY in `app_config.h`. Configuration changes only — no code modifications needed.

```
lab2_phy_update/
├── starter/
│   ├── central/      # TODO comments in app_config.h
│   └── peripheral/   # TODO comments in app_config.h
└── solution/
    ├── central/      # PHY_UPDATE=1, PHY=2M
    └── peripheral/   # PHY_UPDATE=1, PHY=2M
```

## Configuration File

The student-facing configuration is centralized in `app_config.h` in each project. This file contains:

- **Feature flags** — enable/disable protocol features (`APP_ENABLE_PHY_UPDATE`, etc.)
- **PHY selection** — choose between 1M and 2M PHY
- **Link layer parameters** — connection interval, DLE
- **GATT parameters** — MTU size

## Build Requirements

- [ModusToolbox 3.x](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
- GCC ARM toolchain (included with ModusToolbox)

