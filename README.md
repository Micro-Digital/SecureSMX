# SecureSMX Secure RTOS

Starting with v6.0 the license is dual GPLv2 or commercial.
For our commercial license, contact sales@smxrtos.com.

SecureSMX enables dividing an application into fully isolated partitions.
See www.smxrtos.com/securesmx for background.

Note that it can be configured to just SMX RTOS if security features
are not desired. See the note about SMX_CFG_SSMX in the release notes,
in DOC\smx60.txt.

Release notes have information about boards supported and configuration.
We recommend using STMicro STM32F746G-Discovery (Cortex-M7), which is
commonly available for about $50 (e.g. DigiKey STM32F746G-DISCO).
Select the board in CFG\iararm.h and open the corresponding project file
in the APP directory.

Manuals are in DOC\Manuals. The SecureSMX manual supplements the SMX manuals.

IAR Embedded Workbench for Arm v9.40.2 or later is required. We mainly work
with this version currently, but we also tested quickly with v9.70.1.

See license.txt and welcome.txt.

The executables (smxAware in SA and utilities in BIN) cannot be built from
source code in this repo, so they are supplied as .exe files. Including them
here rather than posting on our FTP site is more convenient and ensures they
(esp smxAware) are matched to the code, as it is changed.

Support services are offered by Micro Digital. These include porting to a
new MCU and eval board that you want to use, developing a framework for your
application, and general support and help. Inquire at support@smxrtos.com.