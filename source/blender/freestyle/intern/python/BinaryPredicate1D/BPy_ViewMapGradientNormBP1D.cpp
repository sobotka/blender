/*
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301, USA.
 */

/** \file
 * \ingroup freestyle
 */

#include "BPy_ViewMapGradientNormBP1D.h"

#include "../BPy_Convert.h"
#include "../BPy_IntegrationType.h"

#ifdef __cplusplus
extern "C" {
#endif

using namespace Freestyle;

///////////////////////////////////////////////////////////////////////////////////////////

//------------------------INSTANCE METHODS ----------------------------------

// ViewMapGradientNormBP1D(int level, IntegrationType iType=MEAN, float sampling=2.0)

static char ViewMapGradientNormBP1D___doc__[] =
    "Class hierarchy: :class:`freestyle.types.BinaryPredicate1D` > "
    ":class:`ViewMapGradientNormBP1D`\n"
    "\n"
    ".. method:: __init__(level, integration_type=IntegrationType.MEAN, sampling=2.0)\n"
    "\n"
    "   Builds a ViewMapGradientNormBP1D object.\n"
    "\n"
    "   :arg level: The level of the pyramid from which the pixel must be\n"
    "      read.\n"
    "   :type level: int\n"
    "   :arg integration_type: The integration method used to compute a single value\n"
    "      from a set of values.\n"
    "   :type integration_type: :class:`freestyle.types.IntegrationType`\n"
    "   :arg sampling: The resolution used to sample the chain:\n"
    "      GetViewMapGradientNormF0D is evaluated at each sample point and\n"
    "      the result is obtained by combining the resulting values into a\n"
    "      single one, following the method specified by integration_type.\n"
    "   :type sampling: float\n"
    "\n"
    ".. method:: __call__(inter1, inter2)\n"
    "\n"
    "   Returns true if the evaluation of the Gradient norm Function is\n"
    "   higher for inter1 than for inter2.\n"
    "\n"
    "   :arg inter1: The first Interface1D object.\n"
    "   :type inter1: :class:`freestyle.types.Interface1D`\n"
    "   :arg inter2: The second Interface1D object.\n"
    "   :type inter2: :class:`freestyle.types.Interface1D`\n"
    "   :return: True or false.\n"
    "   :rtype: bool\n";

static int ViewMapGradientNormBP1D___init__(BPy_ViewMapGradientNormBP1D *self,
                                            PyObject *args,
                                            PyObject *kwds)
{
  static const char *kwlist[] = {"level", "integration_type", "sampling", nullptr};
  PyObject *obj = nullptr;
  int i;
  float f = 2.0;

  if (!PyArg_ParseTupleAndKeywords(
          args, kwds, "i|O!f", (char **)kwlist, &i, &IntegrationType_Type, &obj, &f)) {
    return -1;
  }
  IntegrationType t = (obj) ? IntegrationType_from_BPy_IntegrationType(obj) : MEAN;
  self->py_bp1D.bp1D = new Predicates1D::ViewMapGradientNormBP1D(i, t, f);
  return 0;
}

/*-----------------------BPy_ViewMapGradientNormBP1D type definition ----------------------------*/

PyTypeObject ViewMapGradientNormBP1D_Type = {
    PyVarObject_HEAD_INIT(nullptr, 0) "ViewMapGradientNormBP1D", /* tp_name */
    sizeof(BPy_ViewMapGradientNormBP1D),                         /* tp_basicsize */
    0,                                                           /* tp_itemsize */
    nullptr,                                                     /* tp_dealloc */
#if PY_VERSION_HEX >= 0x03080000
    0, /* tp_vectorcall_offset */
#else
    nullptr, /* tp_print */
#endif
    nullptr,                                    /* tp_getattr */
    nullptr,                                    /* tp_setattr */
    nullptr,                                    /* tp_reserved */
    nullptr,                                    /* tp_repr */
    nullptr,                                    /* tp_as_number */
    nullptr,                                    /* tp_as_sequence */
    nullptr,                                    /* tp_as_mapping */
    nullptr,                                    /* tp_hash  */
    nullptr,                                    /* tp_call */
    nullptr,                                    /* tp_str */
    nullptr,                                    /* tp_getattro */
    nullptr,                                    /* tp_setattro */
    nullptr,                                    /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE,   /* tp_flags */
    ViewMapGradientNormBP1D___doc__,            /* tp_doc */
    nullptr,                                    /* tp_traverse */
    nullptr,                                    /* tp_clear */
    nullptr,                                    /* tp_richcompare */
    0,                                          /* tp_weaklistoffset */
    nullptr,                                    /* tp_iter */
    nullptr,                                    /* tp_iternext */
    nullptr,                                    /* tp_methods */
    nullptr,                                    /* tp_members */
    nullptr,                                    /* tp_getset */
    &BinaryPredicate1D_Type,                    /* tp_base */
    nullptr,                                    /* tp_dict */
    nullptr,                                    /* tp_descr_get */
    nullptr,                                    /* tp_descr_set */
    0,                                          /* tp_dictoffset */
    (initproc)ViewMapGradientNormBP1D___init__, /* tp_init */
    nullptr,                                    /* tp_alloc */
    nullptr,                                    /* tp_new */
};

///////////////////////////////////////////////////////////////////////////////////////////

#ifdef __cplusplus
}
#endif
