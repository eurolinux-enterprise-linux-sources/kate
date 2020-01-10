// This file is part of Pate, Kate' Python scripting plugin.
//
// A couple of useful macros and functions used inside of pate_engine.cpp and pate_plugin.cpp.
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

#ifndef PATE_UTILITIES_H
#define PATE_UTILITIES_H

#include "Python.h"

class QLibrary;
class QString;
class KConfigBase;

// save us some ruddy time when printing out QStrings with UTF-8
#define PQ(x) x.toUtf8().constData()

namespace Pate
{

/**
 * Instantiate this class on the stack to automatically get and release the
 * GIL.
 *
 * Also, making all the utility functions members of this class means that in
 * many cases the compiler tells us where the class in needed. In the remaining
 * cases (i.e. bare calls to the Python C API), inspection is used to needed
 * to add the requisite Python() object. To prevent this object being optimised
 * away in these cases due to lack of use, all instances have the form of an
 * assignment, e.g.:
 *
 *      Python py = Python()
 *
 * This adds a little overhead, but this is a small price for consistency.
 */
class Python
{
public:
    Python();
    ~Python();

    /**
     * Load the Python interpreter.
     */
    static void libraryLoad();

    /**
     * Unload the Python interpreter.
     */
    static void libraryUnload();

    /// Convert a QString to a Python unicode object.
    static PyObject *unicode(const QString &string);

    /// Convert a Python unicode object to a QString.
    static QString unicode(PyObject *string);

    /// Test if a Python object is compatible with a QString.
    static bool isUnicode(PyObject *string);

    /// Prepend a QString to a list as a Python unicode object
    bool prependStringToList(PyObject *list, const QString &value);

    /**
     * Print and save (see @ref lastTraceback()) the current traceback in a
     * form approximating what Python would print:
     *
     * Traceback (most recent call last):
     *   File "/home/shahhaqu/.kde/share/apps/kate/pate/pluginmgr.py", line 13, in <module>
     *     import kdeui
     * ImportError: No module named kdeui
     * Could not import pluginmgr.
     */
    void traceback(const QString &description);

    /**
     * Store the last traceback we handled using @ref traceback().
     */
    const QString &lastTraceback(void) const;

    /**
     * Create a Python dictionary from a KConfigBase instance, writing the
     * string representation of the values.
     */
    void updateDictionaryFromConfiguration(PyObject *dictionary, const KConfigBase *config);

    /**
     * Write a Python dictionary to a configuration object, converting objects
     * to their string representation along the way.
     */
    void updateConfigurationFromDictionary(KConfigBase *config, PyObject *dictionary);

    static const char *PATE_ENGINE;

    /**
     * Call the named module's named entry point.
     */
    bool functionCall(const char *functionName, const char *moduleName = PATE_ENGINE);

    /**
     * Delete the item from the named module's dictionary.
     */
    bool itemStringDel(const char *item, const char *moduleName = PATE_ENGINE);

    /**
     * Get the item from the named module's dictionary.
     *
     * @return 0 or a borrowed reference to the item.
     */
    PyObject *itemString(const char *item, const char *moduleName = PATE_ENGINE);

    /**
     * Get the item from the given dictionary.
     *
     * @return 0 or a borrowed reference to the item.
     */
    PyObject *itemString(const char *item, PyObject *dict);

    /**
     * Set the item in the named module's dictionary.
     */
    bool itemStringSet(const char *item, PyObject *value, const char *moduleName = PATE_ENGINE);

    /**
     * Get the Actions defined by a module. The returned object is
     * [ { function, ( text, icon, shortcut, menu ) }... ] for each module
     * function decorated with @action.
     *
     * @return 0 or a new reference to the result.
     */
    PyObject *moduleActions(const char *moduleName);

    /**
     * Get the ConfigPages defined by a module. The returned object is
     * [ { function, callable, ( name, fullName, icon ) }... ] for each module
     * function decorated with @configPage.
     *
     * @return 0 or a new reference to the result.
     */
    PyObject *moduleConfigPages(const char *moduleName);

    /**
     * Get the named module's dictionary.
     *
     * @return 0 or a borrowed reference to the dictionary.
     */
    PyObject *moduleDict(const char *moduleName = PATE_ENGINE);

    /**
     * Get the help text defined by a module.
     */
    QString moduleHelp(const char *moduleName);

    /**
     * Import the named module.
     *
     * @return 0 or a borrowed reference to the module.
     */
    PyObject *moduleImport(const char *moduleName);

    /**
     * A void * for an arbitrary Qt/KDE object that has been wrapped by SIP. Nifty.
     *
     * @param o         The object to be unwrapped. The reference is borrowed.
     */
    void *objectUnwrap(PyObject *o);

    /**
     * A PyObject * for an arbitrary Qt/KDE object using SIP wrapping. Nifty.
     *
     * @param o         The object to be wrapped.
     * @param className The full class name of o, e.g. "PyQt4.QtGui.QWidget".
     * @return 0 or a new reference to the object.
     */
    PyObject *objectWrap(void *o, QString className);

private:
    static QLibrary *s_pythonLibrary;
    static PyThreadState *s_pythonThreadState;
    PyGILState_STATE m_state;
    QString m_traceback;

    /**
     * Run a handler function supplied by the kate module on another module.
     *
     * @return 0 or a new reference to the result.
     */
    PyObject *kateHandler(const char *moduleName, const char *handler);
    PyObject *functionCall(const char *functionName, const char *moduleName, PyObject *arguments);
};

} // namespace Pate

#endif
