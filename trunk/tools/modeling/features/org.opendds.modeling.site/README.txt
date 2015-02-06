0. You should already have a workspace containing the projects in
$DDS_ROOT/tools/modeling/plugins and $DDS_ROOT/tools/modeling/features.
If not, start with an empty workspace and run File -> Import ->
"Existing Projects into Workspace" with root directory $DDS_ROOT/tools/modeling.

1. Make sure the feature project (org.opendds.modeling.feature) feature.xml
lists all our plugins that users should install, and that its dependency
list is up to date.

2. If existing users' installs should be updated, the version number needs to
be incremented (per-plugin) -- the "qualifier" change is not enough.
  * Our policy is to increment the version numbers of all plugins together in
    lock step, even though some plugins may not be changing.  This reduces
    the potential for user confusion by keeping a single version number.
  * Version numbers are of the form X.Y.Z.qualifier, so the feature/plugin
    versions will have a "0" for the "Z" part even when OpenDDS itself does not.
  * see $DDS_ROOT/tools/modeling/update_version.pl
  * Commit any version number changes to the OpenDDS repository.

3. Open site.xml from org.opendds.modeling.site and click the "Build All" button
inside the site.xml editor.

4. Create a zip file for offline updates.
cd $DDS_ROOT/tools/modeling/features/org.opendds.modeling.site
rm content.jar artifacts.jar   # these are for an older Eclipse site format
zip -r opendds_modeling_site . -x README.txt '*.svn/*' .project

5. Update the opendds.org web site.  Run the following commands with
$WEBSITE replaced by the root of the opendds.org subversion checkout.
$ECLIPSE_VER replaced by version-specific directory. The eclipse subdirectory
contains jars compatible with eclipse 3.5 and the eclipse_44 subdirectory
those compatible with eclipse 4.4.  Create a new subdirectory when
incompatible changes are made.
tar --exclude=README.txt --exclude='.*' -ch * | tar -C $WEBSITE/modeling/$ECLIPSE_VER -x
svn revert site.xml
# revert because Eclipse replaced "qualifier" by the date stamp, don't commit
cd $WEBSITE/modeling/$ECLIPSE_VER
svn add features/*.jar plugins/*.jar
# a few of those may already be added, so ignore svn's complaints about them
svn commit

6. Now sync the web site from subversion to the live site, which is beyond
the scope of this document.
