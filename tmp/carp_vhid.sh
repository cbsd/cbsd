vhid_add="1 2"

vhid_advskew_1="1"
vhid_pass_1="pass"
vhid_interface_1="auto"
vhid_state_1="master"

vhid_advskew_2="2"
vhid_pass_2="pass"
vhid_interface_2="auto"
vhid_state_2="master"


insert_vhid()
{
cat > /dev/stdout <<EOF
INSERT INTO carp ( advskew, pass, interface, state ) VALUES ( ${advskew}, "${pass}", "${interface}", "${state}" );
EOF
}


for i in ${vhid_add}; do
	unset vhid advskew pass interface state
	vhid="${i}"
	eval advskew=\$vhid_advskew_$i
	eval pass=\$vhid_pass_$i
	eval interface=\$vhid_interface_$i
	eval state=\$vhid_state_$i

	insert_vhid

done


