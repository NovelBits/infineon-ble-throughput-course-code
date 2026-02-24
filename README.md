# Bluetooth LE Data Throughput Course — Lab Code

Course code for the **Bluetooth LE Data Throughput** course using Infineon CYW920829M2EVK-02 evaluation kits.

The repository contains a single central and peripheral project. You work through the labs by editing `app_config.h` in each project — enabling feature flags and adjusting parameters as directed by the course. Reference configurations for each lab are provided in `configs/`.

## Repository Structure

```
infineon-ble-throughput-course-code/
├── central/            # Central (initiator) — ModusToolbox project
├── peripheral/         # Peripheral (advertiser) — ModusToolbox project
├── configs/            # Reference app_config.h for each lab
│   ├── lab1-central.h
│   ├── lab1-peripheral.h
│   ├── lab2-central.h
│   ├── lab2-peripheral.h
│   ├── lab3-central.h
│   └── lab3-peripheral.h
└── mtb_shared/         # Shared ModusToolbox libraries
```

## Hardware Required

- 2x [Infineon CYW920829M2EVK-02 Evaluation Kits](https://www.infineon.com/cms/en/product/evaluation-boards/cyw920829m2evk-02/)
- 1x [CY8CKIT-028-SENSE IoT Sense Expansion Shield](https://www.infineon.com/cms/en/product/evaluation-boards/cy8ckit-028-sense/) (optional, for OLED display)
- Micro-USB cables for both boards

## Getting Started

### 1. Clone this repository

```bash
git clone https://github.com/NovelBits/infineon-ble-throughput-course-code.git
```

### 2. Import projects into ModusToolbox

1. Open **Eclipse IDE for ModusToolbox**
2. Click **Import Existing Application In-Place** from the Quick Panel
3. Browse to `central/` and import the project
4. Repeat for `peripheral/`
5. The IDE will run `make getlibs` to fetch library dependencies (requires internet). The Board Support Package (BSP) is already included in the repository under `bsps/`.

### 3. Set probe serial numbers

Each project's Makefile has a commented-out placeholder:

```makefile
# MTB_PROBE_SERIAL=
```

Uncomment and add your probe serial for each board so the IDE flashes the correct board. See the course instructions (Lesson 1.3) for how to find your probe serials.

### 4. Build and flash

1. Select the project in the Project Explorer
2. Click **Build** (hammer icon)
3. Connect the target board via micro-USB
4. Click **Program** (green play icon with chip)

## Lab Workflow

Instead of switching between separate project folders, you work with the **same two projects** throughout the course. Each lab directs you to edit `app_config.h` — enabling feature flags and adjusting parameter values.

| Lab | What You Change in `app_config.h` | Reference Config |
|-----|-----------------------------------|-----------------|
| **Lab 1** | Nothing — baseline with all flags OFF | `configs/lab1-*.h` |
| **Lab 2** | Enable `APP_ENABLE_PHY_UPDATE`, select 2M PHY | `configs/lab2-*.h` |
| **Lab 3** | Enable `APP_ENABLE_CONN_PARAM_UPDATE`, set CI = 7.5 ms, DLE = 251 | `configs/lab3-*.h` |

### Using Reference Configs

If you get stuck or want to reset to a known-good state for a lab, copy the matching reference config:

```bash
cp configs/lab2-central.h central/app_config.h
cp configs/lab2-peripheral.h peripheral/app_config.h
```

Then rebuild and reflash both boards.

## Configuration File

The student-facing configuration is centralized in `app_config.h` in each project. This file contains:

- **Feature flags** — enable/disable protocol features (`APP_ENABLE_PHY_UPDATE`, `APP_ENABLE_CONN_PARAM_UPDATE`, `APP_ENABLE_MTU_EXCHANGE`)
- **PHY selection** — choose between 1M and 2M PHY
- **Link layer parameters** — connection interval, DLE
- **GATT parameters** — MTU size

## Build Requirements

- [ModusToolbox 3.x](https://www.infineon.com/cms/en/design-support/tools/sdk/modustoolbox-software/)
- GCC ARM toolchain (included with ModusToolbox)
