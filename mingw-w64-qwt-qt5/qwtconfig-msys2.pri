################################################################
# Qwt Widget Library
# Copyright (C) 1997   Josef Wilgen
# Copyright (C) 2002   Uwe Rathmann
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the Qwt License, Version 1.0
################################################################

QWT_VER_MAJ      = 6
QWT_VER_MIN      = 1
QWT_VER_PAT      = 1
QWT_VERSION      = $${QWT_VER_MAJ}.$${QWT_VER_MIN}.$${QWT_VER_PAT}

######################################################################
# Install paths
######################################################################

#QWT_INSTALL_PREFIX = $$[QT_INSTALL_PREFIX]
QWT_INSTALL_PREFIX = %VALUE_QWT_INSTALL_PREFIX%

QWT_INSTALL_DOCS      = $${QWT_INSTALL_PREFIX}/share/doc/qwt
QWT_INSTALL_HEADERS   = $${QWT_INSTALL_PREFIX}/include/qwt
QWT_INSTALL_LIBS      = $${QWT_INSTALL_PREFIX}/lib

######################################################################
# Designer plugin
######################################################################

QWT_INSTALL_PLUGINS   = $${QWT_INSTALL_PREFIX}/plugins/designer

######################################################################
# Features
# When building a Qwt application with qmake you might want to load
# the compiler/linker flags, that are required to build a Qwt application
# from qwt.prf. Therefore all you need to do is to add "CONFIG += qwt" 
# to your project file and take care, that qwt.prf can be found by qmake.
# ( see http://doc.trolltech.com/4.7/qmake-advanced-usage.html#adding-new-configuration-features )
# I recommend not to install the Qwt features together with the
# Qt features, because you will have to reinstall the Qwt features,
# with every Qt upgrade. 
######################################################################

QWT_INSTALL_FEATURES  = $${QWT_INSTALL_PREFIX}/mkspecs/features

######################################################################
# Build the static/shared libraries.
# If QwtDll is enabled, a shared library is built, otherwise
# it will be a static library.
######################################################################

QWT_CONFIG           += QwtDll

######################################################################
# QwtPlot enables all classes, that are needed to use the QwtPlot 
# widget. 
######################################################################

QWT_CONFIG       += QwtPlot

######################################################################
# QwtWidgets enables all classes, that are needed to use the all other
# widgets (sliders, dials, ...), beside QwtPlot. 
######################################################################

QWT_CONFIG     += QwtWidgets

######################################################################
# If you want to display svg images on the plot canvas, or
# export a plot to a SVG document
######################################################################

QWT_CONFIG     += QwtSvg

######################################################################
# You can use the MathML renderer of the Qt solutions package to 
# enable MathML support in Qwt. Because of license implications
# the ( modified ) code of the MML Widget solution is included and
# linked together with the QwtMathMLTextEngine into an own library. 
# To use it you will have to add "CONFIG += qwtmathml"
# to your qmake project file.
######################################################################

#QWT_CONFIG     += QwtMathML

######################################################################
# If you want to build the Qwt designer plugin, 
# enable the line below.
# Otherwise you have to build it from the designer directory.
######################################################################

QWT_CONFIG     += QwtDesigner

######################################################################
# If you want to auto build the examples, enable the line below
# Otherwise you have to build them from the examples directory.
######################################################################

#QWT_CONFIG     += QwtExamples

######################################################################
# When Qt has been built as framework qmake ( qtAddLibrary ) wants 
# to link frameworks instead of regular libs
######################################################################

macx:CONFIG(qt_framework, qt_framework|qt_no_framework) {

    QWT_CONFIG += QwtFramework
}