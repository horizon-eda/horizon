#!/usr/bin/env bash
DISTDIR=dist/horizon
rm -rf dist
mkdir -p $DISTDIR
cp build/horizon-{eda,imp}.exe $DISTDIR
strip $DISTDIR/horizon-*
LIBS=(
	libstdc++-6.dll\
	libgcc_s_seh-1.dll\
	libglibmm-2.4-1.dll\
	libglib-2.0-0.dll\
	libgio-2.0-0.dll\
	libgiomm-2.4-1.dll\
	libwinpthread-1.dll\
	libsqlite3-0.dll\
	libgmodule-2.0-0.dll\
	libgobject-2.0-0.dll\
	zlib1.dll\
	libintl-8.dll\
	libsigc-2.0-0.dll\
	libffi-8.dll\
	libiconv-2.dll\
	libpcre2-8-0.dll\
	libatkmm-1.6-1.dll\
	libatk-1.0-0.dll\
	libgtk-3-0.dll\
	libgtkmm-3.0-1.dll\
	libpango-1.0-0.dll\
	libpangomm-1.4-1.dll\
	libcairomm-1.0-1.dll\
	libcairo-2.dll\
	libpangocairo-1.0-0.dll\
	libgdk-3-0.dll\
	libgdkmm-3.0-1.dll\
	libgdk_pixbuf-2.0-0.dll\
	libpangoft2-1.0-0.dll\
	libpangowin32-1.0-0.dll\
	libfontconfig-1.dll\
	libfreetype-6.dll\
	libcairo-gobject-2.dll\
	libepoxy-0.dll\
	libharfbuzz-0.dll\
	libpixman-1-0.dll\
	libpng16-16.dll\
	libexpat-1.dll\
	libbz2-1.dll\
	libgraphite2.dll\
	libjpeg-8.dll\
	librsvg-2-2.dll\
	libxml2-2.dll\
	liblzma-5.dll\
	libtiff-6.dll\
	libzmq.dll\
	libsodium-26.dll\
	libcurl-4.dll\
	libgit2-1.7.dll\
	libidn2-0.dll\
	libssh2-1.dll\
	libunistring-5.dll\
	libTKBO.dll\
	libTKBRep.dll\
	libTKCAF.dll\
	libTKCDF.dll\
	libTKernel.dll\
	libTKG2d.dll\
	libTKG3d.dll\
	libTKGeomAlgo.dll\
	libTKGeomBase.dll\
	libTKHLR.dll\
	libTKLCAF.dll\
	libTKMath.dll\
	libTKMesh.dll\
	libTKPrim.dll\
	libTKService.dll\
	libTKShHealing.dll\
	libTKSTEP.dll\
	libTKSTEP209.dll\
	libTKSTEPAttr.dll\
	libTKSTEPBase.dll\
	libTKTopAlgo.dll\
	libTKV3d.dll\
	libTKXCAF.dll\
	libTKXDESTEP.dll\
	libTKXSBase.dll\
	libbrotlicommon.dll\
	libbrotlidec.dll\
	libfribidi-0.dll\
	libhttp_parser-2.dll\
	libpsl-5.dll\
	libthai-0.dll\
	libdatrie-1.dll\
	libcrypto-3-x64.dll\
	libpodofo.dll\
	libidn-12.dll\
	libarchive-13.dll\
	libzstd.dll\
	liblz4.dll\
	libb2-1.dll\
	libdeflate.dll\
	libwebp-7.dll\
	libjbig-0.dll\
	libLerc.dll\
	libsharpyuv-0.dll\
	gspawn-win64-helper.exe\
	gspawn-win64-helper-console.exe
)
for LIB in "${LIBS[@]}"
do
   cp /mingw64/bin/$LIB $DISTDIR
done

mkdir -p $DISTDIR/share/icons
cp -r /mingw64/share/icons/Adwaita $DISTDIR/share/icons
cp -r /mingw64/share/icons/hicolor $DISTDIR/share/icons
rm -rf $DISTDIR/share/icons/Adwaita/cursors

mkdir -p $DISTDIR/lib
cp -r /mingw64/lib/gdk-pixbuf-2.0 $DISTDIR/lib
gdk-pixbuf-query-loaders > $DISTDIR/lib/gdk-pixbuf-2.0/2.10.0/loaders.cache
rm $DISTDIR/lib/gdk-pixbuf-*/*/loaders/*.a

mkdir -p $DISTDIR/share/glib-2.0/schemas
cp /mingw64/share/glib-2.0/schemas/gschemas.compiled $DISTDIR/share/glib-2.0/schemas

cp -r /mingw64/lib/engines-3 $DISTDIR/lib
cp -r /mingw64/lib/ossl-modules $DISTDIR/lib

git log -10 | unix2dos > dist/log.txt
if [ "$1" != "-n" ]; then
	cd dist
	zip -r horizon-$(date +%Y-%m-%d-%H%M).zip horizon log.txt
fi
