for d in */ ; do
	dirname=$(basename "$d")
	cd "$d"
	mkdir -p src
	cat > lib.json << EOF
{
	"name":"$dirname",
	"version":"1.0.0",
	"dependencies":[]
}
EOF
	cd ..
done
echo "fin"
