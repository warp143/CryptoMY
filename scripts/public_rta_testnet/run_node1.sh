DIR="$( cd "$( dirname "${BASH_SOURCE[0]}" )" && pwd )"

${DIR}/../../build/release/bin/cryptomyd --testnet --detach

nohup ${DIR}/../../build/release/bin/cryptomy-supernode ${DIR}/conf_node1.ini > /dev/null 2>&1 &

