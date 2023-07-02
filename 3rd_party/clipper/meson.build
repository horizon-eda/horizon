foreach pic : [true, false]
    clipper_t = static_library('clipper' + (pic?'-pic':''), files('clipper.cpp'), pic:pic)
    if pic
        clipper_pic = clipper_t
    else
        clipper_nopic = clipper_t
    endif
endforeach
