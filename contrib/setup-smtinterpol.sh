mkdir -p ../deps/smtinterpol
cd ../deps/smtinterpol
wget https://ultimate.informatik.uni-freiburg.de/smtinterpol/smtinterpol-2.5-1256-g55d6ba76.jar
echo 'JARPATH=$(dirname $0)' > smtinterpol
echo 'java -jar $JARPATH/smtinterpol-2.5-1256-g55d6ba76.jar -no-success -w -q $1' >> smtinterpol
chmod +x smtinterpol
echo 'export PATH=$PATH:'`pwd` >> ~/.bashrc
