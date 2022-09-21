SET PATH=C:\Keil_v5\ARM\ARMCC\Bin;C:\Program Files\Parallels\Parallels Tools\Applications;C:\Windows\system32;C:\Windows;C:\Windows\System32\Wbem;C:\Windows\System32\WindowsPowerShell\v1.0\;C:\Windows\System32\OpenSSH\;C:\Users\barrywang\AppData\Local\Microsoft\WindowsApps
SET CPU_TYPE=LPC1768
SET CPU_VENDOR=NXP
SET UV2_TARGET=AE Lib RAM
SET CPU_CLOCK=0x00B71B00
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\ae.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\ae_tasks_util.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\ae_timer.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\ae_util.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\main.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmCC" --Via ".\objects\ram\ae_tasks1_g38.__i"
"C:\Keil_v5\ARM\ARMCC\Bin\ArmAr" --Via ".\Objects\RAM\ArInp.Scr"
