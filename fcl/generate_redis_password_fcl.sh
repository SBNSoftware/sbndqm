#!/bin/bash

#Read password and strip newline
REDIS_PASSWORD=$(tr -d '\n' < /home/nfs/sbnd/redis_passfile)

# Do a safe substitution into the FHiCL
sed "s|@REDIS_PASSWORD@|$REDIS_PASSWORD|g" redis_connection_template.fcl > redis_connection_sbnd.fcl
