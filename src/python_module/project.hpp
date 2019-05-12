#pragma once
#include <Python.h>
#include "project/project.hpp"

extern PyTypeObject ProjectType;

class ProjectWrapper {
public:
    ProjectWrapper(const std::string &path);
    horizon::Project project;
};

typedef struct {
    PyObject_HEAD
            /* Type-specific fields go here. */
            ProjectWrapper *project;
} PyProject;
