# GCR_CMake
A CMake extension supporting the **glib-compile-resources** tool.

About
-----

Inspired from the macros for Vala that I used to build a GTK application, I came
to the point where I needed resources for it. For that purpose the
**glib-compile-resources** tool seemed to be the right choice, but the extra XML
file you need to write bothered me. If I add a resource I don't want to mark it
explicitly in CMake and in the XML file. So I came up with this macro that does
everything for you. You just add your resource to a resource list (with
eventually some attributes like compression) and invoke the resource compilation
function. It generates the XML automatically for you. Quite simple, isn't it?

Clone
-----

To clone this repository, just type

```shell
git clone https://github.com/Makman2/GCR_CMake
```

Usage
-----

Just add the macro directory to your `CMAKE_MODULE_PATH`, include the CMake
file and you are ready to go.

```cmake
list(APPEND CMAKE_MODULE_PATH
     ${PATH_TO_GCR_CMAKE_PARENT_DIR}/GCR_CMake/macros)

include(GlibCompileResourcesSupport)
```

Reference
---------

The resource compiling macro is quite powerful and handles as much errors as
possible to make error-finding easier. The function is defined as follows:

```
compile_gresources(<output>
                   <xml_out>
                   [TYPE output_type]
                   [TARGET output_name]
                   [RESOURCES [resources...]]
                   [OPTIONS [command_line_options...]]
                   [PREFIX resource_prefix]
                   [C_PREFIX c_names_prefix]
                   [SOURCE_DIR resource_directory]
                   [COMPRESS_ALL] [NO_COMPRESS_ALL]
                   [STRIPBLANKS_ALL] [NO_STRIPBLANKS_ALL]
                   [TOPIXDATA_ALL] [NO_TOPIXDATA_ALL])
```

- ***output***

  The variable name where to set the output file names. Pass this variable to a
  target as a dependency (i.e. `add_custom_target`).

- ***xml_out***

  The variable name where to store the output file name of the intermediately
  generated gresource-XML-file.

- **TYPE** ***output_type***

  The resource type to generate. Valid values are `EMBED_C`, `EMBED_H`, `BUNDLE`
  or `AUTO`. Anything else will default to `AUTO`.

  - `EMBED_C`: Generate a C code file that can be compiled with your program.

  - `EMBED_H`: Generate a header file to include in your program.

  - `BUNDLE`: Generates a resource disk file to load at runtime.

  - `AUTO` (or anything else): Extract mode from file ending specified in
    `TARGET`.

    If `TARGET` contains
    an invalid file or file ending not detectable, the function results in a
    **FATAL_ERROR**.

    Recognized file formats are: *.gresource*, *.c*, *.h*.

- **TARGET** ***output_name***

  Overrides the default output file name. If not specified (and not
  `AUTO`-**TYPE** is set) the output name is *resources.[type-ending]*.

- **RESOURCES** ***[resources...]***

  The resource list to process. Each resource must be a relative path inside the
  source directory. Each file can be preceded with resource flags.

  - `COMPRESS` flag

    Compress the following file. Effectively sets the attribute *compressed* in
    the XML file to true.

  - `STRIPBLANKS` flag

    Strip whitespace characters in XML files. Sets the *preprocess* attribute in
    XML with *xml-stripblanks* (requires XMLLint).

  - `TOPIXDATA` flag

    Generates a pixdata ready to use in Gdk. Sets the *preprocess* attribute in
    XML with *to-pixdata*.

  Note: Using `STRIPBLANKS` and `TOPIXDATA` together results in a
  **FATAL_ERROR**.

- **OPTIONS** ***command_line_options***

  Extra command line arguments passed to **glib_compile_resources**. For example
  `--internal` or `--manual-register`.

- **PREFIX** ***resource_prefix***

  Overrides the resource prefix. The resource entries get inside the XML a
  prefix that is prepended to each resource file and represents as a whole the
  resource path.

- **C_PREFIX** ***c_names_prefix***
  Specifies the prefix used for the C identifiers in the code generated when
  *EMBED_C* or *EMBED_H* are specified for *TYPE*.

- **SOURCE_DIR** ***resource_directory***

  The source directory where the resource files are. If not overridden, this
  value is set to `CMAKE_CURRENT_SOURCE_DIR`.

- **COMPRESS_ALL**, **NO_COMPRESS_ALL**

  Overrides the `COMPRESS` flag in all resources. If **COMPRESS_ALL** is
  specified, `COMPRESS` is set everywhere regardless of the specified resource
  flags. If **NO_COMPRESS_ALL** is specified, compression is deactivated in all
  resources.

  Specifying **COMPRESS_ALL** and **NO_COMPRESS_ALL** together results in a
  **FATAL_ERROR**.

- **STRIPBLANKS_ALL**, **NO_STRIPBLANKS_ALL**

  Overrides the `STRIPBLANKS` flag in all resources. If **STRIPBLANKS_ALL** is
  specified, `STRIPBLANKS` is set everywhere regardless of the specified
  resource flags. If **NO_STRIPBLANKS_ALL** is specified, stripping away
  whitespaces is deactivated in all resources.

  Specifying **STRIPBLANKS_ALL** and **NO_STRIPBLANKS_ALL** together results in
  a **FATAL_ERROR**.

- **TOPIXDATA_ALL**, **NO_TOPIXDATA_ALL**

  Overrides the `TOPIXDATA` flag in all resources. If **TOPIXDATA_ALL** is
  specified, `TOPIXDATA` is set everywhere regardless of the specified resource
  flags. If **NO_TOPIXDATA_ALL** is specified, converting into pixbufs is
  deactivated in all resources.

  Specifying **TOPIXDATA_ALL** and **NO_TOPIXDATA_ALL** together results in a
  **FATAL_ERROR**.

Kickstart
---------

This is a quick start guide to provide you an easy start with this macro.

Starting with a simple example:

```cmake
set(RESOURCE_LIST
    info.txt
    img/image1.jpg
    img/image2.jpg
    data.xml)

compile_gresources(RESOURCE_FILE
                   XML_OUT
                   TYPE BUNDLE
                   RESOURCES ${RESOURCE_LIST})
```

What does this snippet do? First it sets some resource files to pack into a
resource file. They are located in the source directory passed to CMake at
invocation (`CMAKE_CURENT_SOURCE_DIR`).
After that we compile the resources. Means we generate a *.gresource.xml*-file
(it's path is put inside the `XML_OUT` variable) automatically from our
`RESOURCE_LIST` and create a custom command that compiles the generated
*.gresource.xml*-file with the provided resources into a resource bundle. Since
no specific output file is specified via **TARGET** the output file is placed
into the `CMAKE_CURENT_BINARY_DIR` with the name *resources.gresource*. The
first argument `RESOURCE_FILE` is a variable that is filled with the output file
name, so with *resources.gresource* inside the build directory. This variable is
helpful to create makefile targets (or to process the output file differently).

So here comes a full *CMakeLists.txt* that creates the resources from before.

```cmake
# Minimum support is guaranteed for CMake 2.8. Everything below needs to be
# tested.
cmake_minimum_required(2.8)

project(MyResourceFile)

# Include the extension module.
list(APPEND CMAKE_MODULE_PATH
     ${PATH_TO_GCR_CMAKE_PARENT_DIR}/GCR_CMake/macros)

include(GlibCompileResourcesSupport)

# Set the resources to bundle.
set(RESOURCE_LIST
    info.txt
    img/image1.jpg
    img/image2.jpg
    data.xml)

# Compile the resources.
compile_gresources(RESOURCE_FILE
                   XML_OUT
                   TYPE BUNDLE
                   RESOURCES ${RESOURCE_LIST})

# Add a custom target to the makefile. Now make builds our resource file.
# It depends on the output RESOURCE_FILE.
add_custom_target(resource ALL DEPENDS ${RESOURCE_FILE})
```

A nice feature of the `compile_gresources`-macro is that it supports
individually setting flags on the resources. So we can extend our resource list
like that:

```cmake
set(RESOURCE_LIST
    info.txt
    COMPRESS img/image1.jpg
    COMPRESS img/image2.jpg
    STRIPBLANKS data.xml)
```

This resource list not only simply includes the resources, it specifies also
that the two images should be compressed and in *data.xml* the whitespaces
should be removed. This resource list will include the same files but will
preprocess some of them.

Very handy are the `COMPRESS_ALL`, `STRIPBLANKS_ALL` or `TOPIXDATA_ALL`
parameters (and their `NO_`-equivalents). If you are too lazy to write before
every file the flag, just invoke `compile_gresources` with them.

```cmake
# Compile the resources. Compresses all files regardless if you specified it
# explicitly or not.
compile_gresources(RESOURCE_FILE
                   XML_OUT
                   TYPE BUNDLE
                   RESOURCES ${RESOURCE_LIST}
                   COMPRESS_ALL)
```

So that's a short introduction into the operating mode of the
`compile-gresources` macro.
