
# just to check the format/validity of the source YCSB data
function generate_load_raw {
    # $1 = filename prefix for a "load" trace
    # $2 = (UNUSED)
    # $3 = arguments to YCSB

    echo YCSB load generating $1.load.raw

    ${YCSB_DIR}/bin/ycsb load basic $3 -s > $1.load.raw
}

function generate_load {
    # $1 = filename prefix for a "load" trace
    # $2 = arguments to preprocessTrace
    # $3 = arguments to YCSB

    echo YCSB load generating $1.load

    ${YCSB_DIR}/bin/ycsb load basic $3 -s | ../preprocessTrace $2 $1.load
}

function generate_trans {
    # $1 = filename prefix for a "transaction" trace
    # $2 = arguments to preprocessTrace
    # $3 = arguments to YCSB

    echo YCSB run generating $1.trans

    ${YCSB_DIR}/bin/ycsb run basic $3 -s | ../preprocessTrace $2 $1.trans
}

if [ $# -ne 1 ] 
then 
    echo "Usage: $0 [path-to-YSCB]"
    echo "  e.g. $0 /Users/binfan/projects/fawn/YCSB"
    exit 1
fi

YCSB_DIR=$1

