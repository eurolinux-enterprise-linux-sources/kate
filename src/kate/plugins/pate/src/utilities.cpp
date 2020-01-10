// This file is part of Pate, Kate' Python scripting plugin.
//
// Copyright (C) 2006 Paul Giannaros <paul@giannaros.org>
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) version 3.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public License
// along with this library; see the file COPYING.LIB.  If not, write to
// the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
// Boston, MA 02110-1301, USA.

// config.h defines PATE_PYTHON_LIBRARY, the path to libpython.so
// on the build system
#include "config.h"
#include "Python.h"

#include <QLibrary>
#include <QString>
#include <QStringList>

#include <kconfigbase.h>
#include <kconfiggroup.h>
#include <kdebug.h>
#include <KLocale>

#include "utilities.h"

#define THREADED 1

namespace Pate
{
const char *Python::PATE_ENGINE = "pate";
QLibrary *Python::s_pythonLibrary = 0;
PyThreadState *Python::s_pythonThreadState = 0;

Python::Python()
{
#if THREADED
    m_state = PyGILState_Ensure();
#endif
}

Python::~Python()
{
#if THREADED
    PyGILState_Release(m_state);
#endif
}

bool Python::prependStringToList(PyObject *list, const QString &value)
{
    PyObject *u = unicode(value);
    bool result = 0 == PyList_Insert(list, 0, u);
    Py_DECREF(u);
    if (!result) {
        traceback(QString("Failed to prepend %1").arg(value));
    }
    return result;
}

bool Python::functionCall(const char *functionName, const char *moduleName)
{
    PyObject *result = functionCall(functionName, moduleName, PyTuple_New(0));
    if (!result) {
        return false;
    }
    Py_DECREF(result);
    return true;
}

PyObject *Python::functionCall(const char *functionName, const char *moduleName, PyObject *arguments)
{
    if (!arguments) {
        kError() << "Missing arguments for" << moduleName << functionName;
        return 0;
    }
    PyObject *func = itemString(functionName, moduleName);
    if (!func) {
        kError() << "Failed to resolve" << moduleName << functionName;
        return 0;
    }
    if (!PyCallable_Check(func)) {
        traceback(QString("Not callable %1.%2").arg(moduleName).arg(functionName));
        return 0;
    }
    PyObject *result = PyObject_CallObject(func, arguments);
    Py_DECREF(arguments);
    if (!result) {
        traceback(QString("No result from %1.%2").arg(moduleName).arg(functionName));
    }
    return result;
}

bool Python::itemStringDel(const char *item, const char *moduleName)
{
    PyObject *dict = moduleDict(moduleName);
    if (!dict) {
        return false;
    }
    if (!PyDict_DelItemString(dict, item)) {
        return true;
    }
    traceback(QString("Could not delete item string %1.%2").arg(moduleName).arg(item));
    return false;
}

PyObject *Python::itemString(const char *item, const char *moduleName)
{
    PyObject *value = itemString(item, moduleDict(moduleName));
    if (value) {
        return value;
    }
    kError() << "Could not get item string" << moduleName << item;
    return 0;
}

PyObject *Python::itemString(const char *item, PyObject *dict)
{
    if (!dict) {
        return 0;
    }
    PyObject *value = PyDict_GetItemString(dict, item);
    if (value) {
        return value;
    }
    traceback(QString("Could not get item string %1").arg(item));
    return 0;
}

bool Python::itemStringSet(const char *item, PyObject *value, const char *moduleName)
{
    PyObject *dict = moduleDict(moduleName);
    if (!dict) {
        return false;
    }
    if (PyDict_SetItemString(dict, item, value)) {
        traceback(QString("Could not set item string %1.%2").arg(moduleName).arg(item));
        return false;
    }
    return true;
}

PyObject *Python::kateHandler(const char *moduleName, const char *handler)
{
    PyObject *module = moduleImport(moduleName);
    if (!module) {
        return 0;
    }
    PyObject *arguments = Py_BuildValue("(O)", (PyObject *)module);
    PyObject *result = functionCall(handler, "kate", arguments);
    if (!result) {
        return 0;
    }
    return result;
}

const QString &Python::lastTraceback(void) const
{
    return m_traceback;
}

void Python::libraryLoad()
{
    if (!s_pythonLibrary) {
        kDebug() << "Creating s_pythonLibrary";
        s_pythonLibrary = new QLibrary(PATE_PYTHON_LIBRARY);
        if (!s_pythonLibrary) {
            kError() << "Could not create" << PATE_PYTHON_LIBRARY;
        }
        s_pythonLibrary->setLoadHints(QLibrary::ExportExternalSymbolsHint);
        if (!s_pythonLibrary->load()) {
            kError() << "Could not load" << PATE_PYTHON_LIBRARY;
        }
        Py_InitializeEx(0);
        if (!Py_IsInitialized()) {
            kError() << "Could not initialise" << PATE_PYTHON_LIBRARY;
        }
#if THREADED
        PyEval_InitThreads();
        s_pythonThreadState = PyGILState_GetThisThreadState();
        PyEval_ReleaseThread(s_pythonThreadState);
#endif
    }
}

void Python::libraryUnload()
{
    if (s_pythonLibrary) {
        // Shut the interpreter down if it has been started.
        if (Py_IsInitialized()) {
#if THREADED
            PyEval_AcquireThread(s_pythonThreadState);
#endif
            //Py_Finalize();
        }
        if (s_pythonLibrary->isLoaded()) {
            s_pythonLibrary->unload();
        }
        delete s_pythonLibrary;
        s_pythonLibrary = 0;
    }
}

PyObject *Python::moduleActions(const char *moduleName)
{
    PyObject *result = kateHandler(moduleName, "moduleGetActions");
    return result;
}

PyObject *Python::moduleConfigPages(const char *moduleName)
{
    PyObject *result = kateHandler(moduleName, "moduleGetConfigPages");
    return result;
}

QString Python::moduleHelp(const char *moduleName)
{
    PyObject *result = kateHandler(moduleName, "moduleGetHelp");
    if (!result) {
        return QString();
    }
    QString r(unicode(result));
    Py_DECREF(result);
    return r;
}

PyObject *Python::moduleDict(const char *moduleName)
{
    PyObject *module = moduleImport(moduleName);
    if (!module) {
        return 0;
    }
    PyObject *dictionary = PyModule_GetDict(module);
    if (dictionary) {
        return dictionary;
    }
    traceback(QString("Could not get dict %1").arg(moduleName));
    return 0;
}

PyObject *Python::moduleImport(const char *moduleName)
{
    PyObject *module = PyImport_ImportModule(moduleName);
    if (module) {
        return module;
    }
    traceback(QString("Could not import %1").arg(moduleName));
    return 0;
}

void *Python::objectUnwrap(PyObject *o)
{
    PyObject *arguments = Py_BuildValue("(O)", o);
    PyObject *result = functionCall("unwrapinstance", "sip", arguments);
    if (!result) {
        return 0;
    }
    void *r = (void *)(ptrdiff_t)PyLong_AsLongLong(result);
    Py_DECREF(result);
    return r;
}

PyObject *Python::objectWrap(void *o, QString fullClassName)
{
    QString classModuleName = fullClassName.section('.', 0, -2);
    QString className = fullClassName.section('.', -1);
    PyObject *classObject = itemString(PQ(className), PQ(classModuleName));
    if (!classObject) {
        return 0;
    }
    PyObject *arguments = Py_BuildValue("NO", PyLong_FromVoidPtr(o), classObject);
    PyObject *result = functionCall("wrapinstance", "sip", arguments);
    if (!result) {
        return 0;
    }
    return result;
}

// Inspired by http://www.gossamer-threads.com/lists/python/python/150924.
void Python::traceback(const QString &description)
{
    m_traceback.clear();
    if (!PyErr_Occurred()) {
        // Return an empty string on no error.
        return;
    }

    PyObject *exc_typ, *exc_val, *exc_tb;
    PyErr_Fetch(&exc_typ, &exc_val, &exc_tb);
    PyErr_NormalizeException(&exc_typ, &exc_val, &exc_tb);

    // Include the traceback.
    if (exc_tb) {
        m_traceback = "Traceback (most recent call last):\n";
        PyObject *arguments = PyTuple_New(1);
        PyTuple_SetItem(arguments, 0, exc_tb);
        PyObject *result = functionCall("format_tb", "traceback", arguments);
        if (result) {
            for (int i = 0, j = PyList_Size(result); i < j; i++) {
                PyObject *tt = PyList_GetItem(result, i);
                PyObject *t = Py_BuildValue("(O)", tt);
                char *buffer;
                if (!PyArg_ParseTuple(t, "s", &buffer)) {
                    break;
                }
                m_traceback += buffer;
            }
            Py_DECREF(result);
        }
        Py_DECREF(exc_tb);
    }

    // Include the exception type and value.
    if (exc_typ) {
        PyObject *temp = PyObject_GetAttrString(exc_typ, "__name__");
        if (temp) {
            m_traceback += unicode(temp);
            m_traceback += ": ";
        }
        Py_DECREF(exc_typ);
    }
    if (exc_val) {
        PyObject *temp = PyObject_Str(exc_val);
        if (temp) {
            m_traceback += unicode(temp);
            m_traceback += "\n";
        }
        Py_DECREF(exc_val);
    }
    m_traceback += description;
    kError() << m_traceback;
}

PyObject *Python::unicode(const QString &string)
{
#if PY_MAJOR_VERSION < 3
    /* Python 2.x. http://docs.python.org/2/c-api/unicode.html */
    PyObject *s = PyString_FromString(PQ(string));
    PyObject *u = PyUnicode_FromEncodedObject(s, "utf-8", "strict");
    Py_DECREF(s);
    return u;
#elif PY_MINOR_VERSION < 3
    /* Python 3.2 or less. http://docs.python.org/3.2/c-api/unicode.html#unicode-objects */
#ifdef Py_UNICODE_WIDE
    return PyUnicode_DecodeUTF16((const char *)string.constData(), string.length() * 2, 0, 0);
#else
    return PyUnicode_FromUnicode(string.constData(), string.length());
#endif
#else /* Python 3.3 or greater. http://docs.python.org/3.3/c-api/unicode.html#unicode-objects */
    return PyUnicode_FromKindAndData(PyUnicode_2BYTE_KIND, string.constData(), string.length());
#endif
}

QString Python::unicode(PyObject *string)
{
#if PY_MAJOR_VERSION < 3
    /* Python 2.x. http://docs.python.org/2/c-api/unicode.html */
    if (PyString_Check(string)) {
        return QString(PyString_AsString(string));
    } else if (PyUnicode_Check(string)) {
        int unichars = PyUnicode_GetSize(string);
#ifdef HAVE_USABLE_WCHAR_T
        return QString::fromWCharArray(PyUnicode_AsUnicode(string), unichars);
#else
#ifdef Py_UNICODE_WIDE
        return QString::fromUcs4((const unsigned int *)PyUnicode_AsUnicode(string), unichars);
#else
        return QString::fromUtf16(PyUnicode_AsUnicode(string), unichars);
#endif
#endif
    } else {
        return QString();
    }
#elif PY_MINOR_VERSION < 3
    /* Python 3.2 or less. http://docs.python.org/3.2/c-api/unicode.html#unicode-objects */
    if (!PyUnicode_Check(string)) {
        return QString();
    }
    int unichars = PyUnicode_GetSize(string);
#ifdef HAVE_USABLE_WCHAR_T
    return QString::fromWCharArray(PyUnicode_AsUnicode(string), unichars);
#else
#ifdef Py_UNICODE_WIDE
    return QString::fromUcs4(PyUnicode_AsUnicode(string), unichars);
#else
    return QString::fromUtf16(PyUnicode_AsUnicode(string), unichars);
#endif
#endif
#else /* Python 3.3 or greater. http://docs.python.org/3.3/c-api/unicode.html#unicode-objects */
    if (!PyUnicode_Check(string)) {
        return QString();
    }
    int unichars = PyUnicode_GetLength(string);
    if (0 != PyUnicode_READY(string)) {
        return QString();
    }
    switch (PyUnicode_KIND(string)) {
    case PyUnicode_1BYTE_KIND:
        return QString::fromLatin1((const char *)PyUnicode_1BYTE_DATA(string), unichars);
        break;
    case PyUnicode_2BYTE_KIND:
        return QString::fromUtf16(PyUnicode_2BYTE_DATA(string), unichars);
        break;
    case PyUnicode_4BYTE_KIND:
        return QString::fromUcs4(PyUnicode_4BYTE_DATA(string), unichars);
        break;
    default:
        return QString();
        break;
    }
#endif
}

bool Python::isUnicode(PyObject *string)
{
#if PY_MAJOR_VERSION < 3
    return PyString_Check(string) || PyUnicode_Check(string);
#else
    return PyUnicode_Check(string);
#endif
}

void Python::updateConfigurationFromDictionary(KConfigBase *config, PyObject *dictionary)
{
    PyObject *groupKey;
    PyObject *groupDictionary;
    Py_ssize_t position = 0;
    while (PyDict_Next(dictionary, &position, &groupKey, &groupDictionary)) {
        if (!isUnicode(groupKey)) {
            traceback(QString("Configuration group name not a string"));
            continue;
        }
        QString groupName = unicode(groupKey);
        if (!PyDict_Check(groupDictionary)) {
            traceback(QString("Configuration group %1 top level key not a dictionary").arg(groupName));
            continue;
        }

        //  There is a group per module.
        KConfigGroup group = config->group(groupName);
        PyObject *key;
        PyObject *value;
        Py_ssize_t x = 0;
        while (PyDict_Next(groupDictionary, &x, &key, &value)) {
            if (!isUnicode(key)) {
                traceback(QString("Configuration group %1 itemKey not a string").arg(groupName));
                continue;
            }
            PyObject *arguments = Py_BuildValue("(Oi)", value, 0);
            PyObject *pickled = functionCall("dumps", "pickle", arguments);
            if (pickled) {
#if PY_MAJOR_VERSION < 3
                QString ascii(unicode(pickled));
#else
                QString ascii(PyBytes_AsString(pickled));
#endif
                group.writeEntry(unicode(key), ascii);
                Py_DECREF(pickled);
            } else {
                kError() << "Cannot write" << groupName << unicode(key) << unicode(PyObject_Str(value));
            }
        }
    }
}

void Python::updateDictionaryFromConfiguration(PyObject *dictionary, const KConfigBase *config)
{
    kDebug() << config->groupList();
    foreach(QString groupName, config->groupList()) {
        KConfigGroup group = config->group(groupName);
        PyObject *groupDictionary = PyDict_New();
        PyDict_SetItemString(dictionary, PQ(groupName), groupDictionary);
        foreach(QString key, group.keyList()) {
            QString pickled = group.readEntry(key);
#if PY_MAJOR_VERSION < 3
            PyObject *arguments = Py_BuildValue("(s)", PQ(pickled));
#else
            PyObject *arguments = Py_BuildValue("(y)", PQ(pickled));
#endif
            PyObject *value = functionCall("loads", "pickle", arguments);
            if (value) {
                PyDict_SetItemString(groupDictionary, PQ(key), value);
                Py_DECREF(value);
            } else {
                kError() << "Cannot read" << groupName << key << pickled;
            }
        }
        Py_DECREF(groupDictionary);
    }
}

} // namespace Pate

