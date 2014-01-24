function init()
    local MYNUMTEST=0
    local word

    for word in string.gfind( MYOPTARG .. " " .. MYARG, "[^%s]+") do MYNUMTEST=MYNUMTEST+1 end

    if ( arg[1]=="--args" ) then
	print ( greeting .. " " .. MYNUMTEST )
	for word in string.gfind( MYOPTARG .. " " .. MYARG, "[^%s]+") do
	    print ( word )
	end
	os.exit(0)
    end

    if ( arg[1]=="--help" )  then
	if not CBSDMODULE then
	    CBSDMODULE="sys"
	end

	print ( "[" .. CBSDMODULE .. "] " .. MYDESC )
	print ( "require: " .. MYARG )
	print ( "opt: " .. MYOPTARG )

	if ADDHELP then print ( ADDHELP ) end
	if EXTHELP then print ( "External help: " .. cbsddocsrc .. "/" .. EXTHELP ) end
	os.exit(0)
    end
end
