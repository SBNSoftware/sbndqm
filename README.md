# sbndqm
SBN Data Quality Monitoring

## Compilation instructions
```
source /software/products/setup
source /software/products_dev/setup

setup mrb

export MRB_PROJECT=sbndqm

mrb newDev  -q e20:prof -v1_00_00

. localProducts_sbndqm_1_00_00_e20_prof/setup

cd srcs/
mrb g sbndaq_online
mrb g sbndqm

mrbsetenv #this command outputs an error message, that should be ignored
unsetup TRACE
mrbsetenv

mrb i -j16
```
