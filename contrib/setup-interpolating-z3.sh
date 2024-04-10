mkdir -p ../deps/interpolating-z3
cd ../deps/interpolating-z3
wget https://github.com/Z3Prover/z3/releases/download/z3-4.7.1/z3-4.7.1-x64-ubuntu-16.04.zip
unzip z3-4.7.1-x64-ubuntu-16.04.zip
cd z3-4.7.1-x64-ubuntu-16.04/bin
mv z3 z3-4.7.1
echo 'export PATH=$PATH:'`pwd` >> ~/.bashrc


