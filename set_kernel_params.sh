#!/bin/sh

# Set the kernel parameters for larger receive and send buffer sizes
sysctl -w net.core.rmem_max=16777216
sysctl -w net.core.wmem_max=16777216

# Start your application or shell as needed
exec "$@"
