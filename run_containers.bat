start cmd /k docker start -i producer
start cmd /k docker start -i broker
start /min cmd /k docker exec -i broker bash -c "bash wsopen"
start cmd /k docker start -i consumer
