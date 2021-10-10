import qbs

CppApplication {
    type: "application" // To suppress bundle generation on Mac
    consoleApplication: true
    files: ['main.c',
            'pathtools.h',
            'pathtools.c',
            'msys2_relocate.h',
            'msys2_relocate.c']
    
    Group {     // Properties for the produced executable
        fileTagsFilter: product.type
        qbs.install: true
    }
}
