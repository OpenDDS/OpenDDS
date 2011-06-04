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

3. Open site.xml from org.opendds.modeling.site and click the "Build All" button
inside the site.xml editor.

4. Create a zip file for offline updates.
cd $DDS_ROOT/tools/modeling/features/org.opendds.modeling.site
rm content.jar artifacts.jar   # these are for an older Eclipse site format
zip -r opendds_modeling_site . -x README.txt .svn .project

NOTE: On an Ubuntu 10.04 using zip 3.0 (built with gcc 4.4), I needed to
      do the following to zip the archive:

  bash> zip -r opendds_modeling_site . -x README.txt .svn/\* web/.svn/\* .project

5. Update the opendds.org web site.  Run the following commands with
$WEBSITE replaced by the root of the opendds.org subversion checkout.
tar --exclude=README.txt --exclude='.*' -ch * | tar -C $WEBSITE/modeling/eclipse -x
svn revert site.xml
# revert because Eclipse replaced "qualifier" by the date stamp, don't commit
cd $WEBSITE/modeling/eclipse
svn add features/*.jar plugins/*.jar
# a few of those may already be added, so ignore svn's complaints about them
svn commit

6. Now sync the web site from subversion to the live site, which is beyond
the scope of this document.
