eval '(exit $?0)' && eval 'exec perl -S $0 ${1+"$@"}'
    & eval 'exec perl -S $0 $argv:q'
    if 0;

# Purpose: Copy icons under this directory to appropriate places in
# the plugins. Currently there are not distinct icons for each modeling
# element. The icon-to-element.txt file in this directory, along with this
# script, will allow the duplicate icons to automatically be set.

use strict;
use File::Copy;
use File::Compare;

my $ELEMENT_ICONS_DIR =
    '../plugins/org.opendds.modeling.model.edit/icons/full/obj16';
my $MODEL_FILE_ICONS_DIR =
    '../plugins/org.opendds.modeling.model.editor/icons/full/obj16';
my $DIAGRAM_FILE_ICONS_DIR =
    '../plugins/org.opendds.modeling.diagram.main/icons/obj16';

sub copy_file {
  my ($source, $target) = @_;
  print "Copy $source to $target...\n";

  if (compare($source, $target) == 0) {
    print "  The source and target are identical\n";
  }
  else {
    copy($source, $target) or die "Copy failed: $!";
    print "  '$source' -> '$target'\n";
  }
}

sub element_icons_set {
  my $file = shift;
  open IN, $file or die "Can't open $file";
  while (<IN>) {
    my ($icon, $element) = split;
    copy_file('obj16/' . $icon, $ELEMENT_ICONS_DIR . '/' . $element . '.gif');
  }
  close IN;
}

element_icons_set('DataLib-icon-to-element.txt');
element_icons_set('DcpsLib-icon-to-element.txt');
element_icons_set('MainDiagram-icon-to-element.txt');
element_icons_set('PolicyLib-icon-to-element.txt');


# Handle icons for files
copy_file('obj16/OpenDDSModelFile.gif',
          $MODEL_FILE_ICONS_DIR . '/OpenDDSModelFile.gif');
copy_file('obj16/OpenDDSDiagramFile.gif',
          $DIAGRAM_FILE_ICONS_DIR . '/OpenDDSDiagramFile.gif');
