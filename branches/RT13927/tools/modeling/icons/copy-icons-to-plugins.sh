#! /bin/bash

# Purpose: Copy icons under this directory to appropriate places in
# the plugins. Currently there are not distinct icons for each modeling
# element. The icon-to-element.txt file in this directory, along with this
# script, will allow the duplicate icons to automatically be set.

ELEMENT_ICONS_DIR=../plugins/org.opendds.modeling.model.edit/icons/full/obj16
MODEL_FILE_ICONS_DIR=../plugins/org.opendds.modeling.model.editor/icons/full/obj16
DIAGRAM_FILE_ICONS_DIR=../plugins/org.opendds.modeling.diagram.main/icons/obj16

function copy_file()
{
    source=$1
    target=$2

    echo "Copy $source to $target if they are different..."

    diff -q $source $target
    if [ $? -ne 0 ]; then
	cp --force --verbose $source $target

    else
	echo "  The source and target are identical"
    fi

    # Add to the repository if necessary
    svn info $target > /dev/null 2>&1
    if [ $? -ne 0 ]; then
	echo "Adding $target to the repository"
	svn add $target
    fi
}

function element_icons_set()
{
    while read icon element; do
	source_icon=obj16/$icon
	element_icon=$ELEMENT_ICONS_DIR/$element.gif
	copy_file $source_icon $element_icon
    done
}

element_icons_set < icon-to-element.txt

# Handle icons for files
copy_file obj16/OpenDDSModelFile.gif $MODEL_FILE_ICONS_DIR/OpenDDSModelFile.gif
copy_file obj16/OpenDDSDiagramFile.gif $DIAGRAM_FILE_ICONS_DIR/OpenDDSDiagramFile.gif
