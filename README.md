# sbndqm
SBN Data Quality Monitoring

## Compilation instructions
```
source /software/products/setup
source /software/products_dev/setup

setup mrb

export MRB_PROJECT=sbndqm

mrb newDev  -q e19:prof -v0_07_04

. localProducts_sbndqm_0_07_04_e19_prof/setup

cd srcs/
mrb g sbndaq_online
mrb g sbndaq_decode
mrb g sbndqm

mrbsetenv #this command outputs an error message, that should be ignored
unsetup TRACE
mrbsetenv

mrb i -j16
```
