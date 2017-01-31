Step by-step guide for getting started with horizon on windows-based 
platforms.

# Install MSYS2
Download and run the msys2 installer from http://msys2.github.io/ I've 
only tested with the 64bit version, 32bit should work as well (but come 
on, it's 2017...) Make sure that the path you select for installation 
doesn't contain any spaces. (Don't blame me for that one)

# Start MSYS console
Launch the Start Menu item "MSYS2 mingw 64 bit" you should be greeted 
with a console window.  All steps below refer to what you should type 
into that window.

# Install updates
Type
```
pacman -Syu
```
if it tells you to close restart msys, close the console window and 
start it again.

# Install dependencies
Type/paste
```
pacman -S mingw-w64-x86_64-gtkmm3 git base-devel mingw-w64-x86_64-yaml-cpp mingw-w64-x86_64-boost mingw-w64-x86_64-sqlite3  mingw-w64-x86_64-toolchain  --needed
```

When prompted, just hit return. Sit back and wait for it to install 
what's almost a complete linux environment.

Before continuing you may change to another directory. It easiest to 
type `cd ` followed by a space  and drop the folder  you want to change to on the window.

# Clone horizon
```
git clone http://github.com/carrotIndustries/horizon
cd horizon
```

# Build it
```
make -j 4
```

You may adjust the number to the number of CPUs in your system to speed 
up compilation. Expect 100% CPU load for several minutes. Due to debug 
symbols the resulting executables are of considerable size.

# Set up the pool
We're still in the directory for horizon, go one level up:
```
cd ..
```

and clone the pool
```
git clone http://github.com/carrotIndustries/horizon-pool
```

and go to the "horizon" directory back again

```
cd horizon
```

Now, we'll need to tell horizon where the pool is

```
export HORIZON_POOL=$(realpath ../horizon-pool)
```

and update the pool database

```
./horizon-pool-update
```
If it completes with "created db from schema", it's alright.

# Actually use horizon
For starters, open the symbol editor and edit a symbol:

```
./horizon-imp -y  ../horizon-pool/symbols/passive/resistors/resistor.json
```

You should see a window with a blue canvas. 

Now you're set up to follow the readme: https://github.com/carrotIndustries/horizon#run

# Notes
When you re-open mingw, don't forget to set the HORIZON_POOL 
environment variable as described above.

# Updating
Fetch the latest changes:
```
git pull
```

and rebuild

```
make
```
