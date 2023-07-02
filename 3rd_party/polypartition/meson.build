foreach pic : [true, false]
    polypartition_t = static_library('polypartition' + (pic?'-pic':''), files('polypartition.cpp'), pic:pic)
    if pic
        polypartition_pic = polypartition_t
    else
        polypartition_nopic = polypartition_t
    endif
endforeach
