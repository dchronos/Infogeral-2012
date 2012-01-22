

This is the Media Mover API module. 

-----------------------------------------------------------------------
 INSTALLATION
-----------------------------------------------------------------------
Install the media_mover module directory under sites/all/modules or 
sites/yoursite/modules

Go to admin > build > modules

Enable the Media Mover modules you want to use

-----------------------------------------------------------------------
 CONFIGURATION
-----------------------------------------------------------------------

Go to admin > build > media mover 
These are the global settings for the Media Mover modules

Individual configurations can be configured when you create a new job.

-----------------------------------------------------------------------
 USAGE
-----------------------------------------------------------------------

Goto admin > build > media_mover to build a configuration

Each configuration can have its own options, so you can make a configuration 
that converts a video, and then another configuration which makes a thumbnail 
for that video.

Media Mover configurations will be run every time cron is run, though you
can run a single configuration by hand if you use the run option at 
admin/media_mover

 Example:
 ------------------------------------
 You have a site where users upload video with their nodes. You want
 these files converted to FLV format automatically. Media Mover can
 find and convert the files, but you will need some additional theming
 and potentially some additional modules.
 
 1) add a new configuration
 2) select "Media Mover node module: Select drupal uploaded files"
 3) select the content types you wish to harvest from and the kinds 
    files you wish to harvest
 4) select "FFmpeg module: convert video"
 5) default options for conversion should work in most cases
 
 Once cron runs, uploaded files will be converted. Files are accessible
 at $node->media_mover. To get the flv file to be shown in a flash 
 player, you can get the file path by $media->mover[X][0]['complete_file']
 where X is the Media Mover configuration id.
 
 You may find XSPF Playlist module and the SWFobject module helpful to 
 display videos.
 
-----------------------------------------------------------------------
 DEVELOPERS
-----------------------------------------------------------------------
 please see http://drupal.org/node/566870
