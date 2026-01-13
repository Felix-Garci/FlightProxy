#!/bin/bash

# 1. Definir el nombre de la sesión y directorios
SESSION="dron"
SESSIONEXISTS=$(tmux list-sessions | grep $SESSION)

# 2. Comprobar si la sesión ya existe para no crearla doble
if [ "$SESSIONEXISTS" = "" ]
then
	# Cojemos la ruta donde esta el ejecutable para calcular rutas relativas a distintas apps
	SCRIPT_DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" &> /dev/null && pwd )"

	TRANSPONDER_PATH="$SCRIPT_DIR/../build_linux/"
	REMOTECTRL_PATH="$SCRIPT_DIR/remote_py/"
	DOCKER_PATH="$SCRIPT_DIR/simulador/"
	MSP_PATH="$SCRIPT_DIR/msp/"


    tmux new-session -d -s $SESSION -n "win1"

    tmux send-keys -t $SESSION:win1 "cd $TRANSPONDER_PATH" C-m
    tmux send-keys -t $SESSION:win1 "echo 'esperando 20s' && sleep 20 && ./TransponderLinux" C-m
	#./TransponderLinux" C-m

    tmux split-window -v -p 80 -t $SESSION:win1
    tmux send-keys -t $SESSION:win1 "cd $REMOTECTRL_PATH" C-m
    tmux send-keys -t $SESSION:win1 "source .venv/bin/activate" C-m 
    tmux send-keys -t $SESSION:win1 "python remotecontroller.py" C-m 


    tmux split-window -v -p 80 -t $SESSION:win1
    tmux send-keys -t $SESSION:win1 "cd $DOCKER_PATH" C-m
	tmux send-keys -t $SESSION:win1 "trap 'docker-compose down' EXIT; docker-compose up" C-m

	tmux split-window -v -p 80 -t $SESSION:win1
    tmux send-keys -t $SESSION:win1 "cd $DOCKER_PATH" C-m
    tmux send-keys -t $SESSION:win1 "xhost + && sleep 10 && ./view_simulation.sh" C-m

	tmux split-window -v -p 80 -t $SESSION:win1
    tmux send-keys -t $SESSION:win1 "cd $MSP_PATH" C-m
	tmux send-keys -t $SESSION:win1 "clear && python manual.py" C-m

	tmux split-window -h -p 50 -t $SESSION:win1
    tmux send-keys -t $SESSION:win1 "cd $DOCKER_PATH && clear" C-m

fi

# si pasamos argumento a attachamos session
if [[ "$1" == "a" ]];then
	if [[ -n "$TMUX" ]];then
		tmux switch-client -t $SESSION
	else
		tmux attach-session -t $SESSION
	fi
fi
