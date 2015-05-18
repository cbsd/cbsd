function init()
	local MYNUMTEST=0
	local word

	if not MYARG then
		MYARG=""
	end

	if not MYOPTARG then
		MYOPTARG=""
	end

	if not ADDHELP then
		ADDHELP=""
	end

	if not MYDESC then
		MYDESC=""
	end

	if not myversion then
		myversion=""
	end

	local f = assert (io.popen ("/usr/local/bin/cbsd -c version 2>/dev/null"))

	local bin_version=""

	for line in f:lines() do
		bin_version=( bin_version .. line )
	end -- for loop


	if ( bin_version ~= myversion ) then
		print( "Warning: CBSD is " .. bin_version .. " while workdir initializated for " .. myversion .. ". Please re-run: cbsd initenv" )
	end

	for word in string.gmatch( MYOPTARG .. " " .. MYARG, "[^%s]+" ) do MYNUMTEST=MYNUMTEST+1 end

	if ( arg[1]=="--args" ) then
		print ( greeting .. " " .. MYNUMTEST )
		for word in string.gmatch( MYOPTARG .. " " .. MYARG, "[^%s]+") do
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

	-- XO wrapper here

	dofile(localcbsdconf)

	local i=1

	while arg[i] do
		local x = string.find( arg[i] , "=", 1, true )
		if x then
			local ARG=string.sub( arg[i], 1, x-1 )
			local VAL=string.sub( arg[i], x+1, -1 )
			local s=ARG .. "='" .. VAL .. "'"
			assert(loadstring(s))()
		end
		i = i + 1
	end

	for word in string.gmatch ( MYARG, "[^%s]+" ) do

		local s= [[
			if not jname then
				exist=0
			else
				exist=1
			end
			]]

		assert(loadstring(s))()

		if ( exist == 0 ) then
			print ( "Please set: " .. word  )
			os.exit(1)
		end
	end
end
