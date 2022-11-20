mkdir build
cd build
mkdir mac
cd mac
cmake ../.. -G Xcode -DBUILD_DEVELOPER=True -DBUILD_PLATFORM_MAC=True
cd ..
mkdir ios
cd ios
cmake ../.. -G Xcode -DBUILD_DEVELOPER=True
cd ..
cd ..
