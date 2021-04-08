/*  
 * Copyright (c) 2003-2021 Ke Hengzhong <kehengzhong@hotmail.com>
 * All rights reserved. See MIT LICENSE for redistribution. 
 */

#include "btype.h"
#include "dynarr.h"
#include "memory.h"
#include "hashtab.h"
#include "strutil.h"
#include "mimetype.h"

typedef struct MimeMgmt_ {

    hashtab_t          * mime_tab;      //key is extname
    hashtab_t          * mimeid_tab;    //key is mimeid 
    hashtab_t          * mimetype_tab;  //key is mime type
    arr_t              * mime_list;

} MimeMgmt;

MimeMgmt * g_mimemgmt = NULL;
int        g_mimemgmt_init = 0;

static MimeItem g_mime[] = {

    { 1000, 100, ".appcache", "text/cache-manifest" },
    { 1010, 110, ".ics", "text/calendar" },
    { 1011, 110, ".ifb", "text/calendar" },
    { 1012, 110, ".vcs", "text/x-vcalendar" },
    { 1013, 110, ".vcal", "text/x-vcalendar" },
    { 1020, 120, ".css", "text/css" },
    { 1030, 130, ".csv", "text/csv" },
    { 1131, 130, ".tsv", "text/tab-separated-values" },
    { 1132, 130, ".tab", "text/tab-separated-values" },
    { 1040, 140, ".html", "text/html" },
    { 1041, 140, ".htm", "text/html" },
    { 1050, 150, ".n3", "text/n3" },
    { 1060, 160, ".txt", "text/plain" },
    { 1061, 160, ".text", "text/plain" },
    { 1070, 170, ".conf", "text/plain" },
    { 1080, 180, ".def", "text/plain" },
    { 1090, 190, ".list", "text/plain" },
    { 1100, 200, ".log", "text/plain" },
    { 1110, 210, ".in", "text/plain" },
    { 1120, 220, ".dsc", "text/prs.lines.tag" },
    { 1130, 230, ".rtx", "text/richtext" },
    { 1140, 240, ".sgml", "text/sgml" },
    { 1141, 240, ".sgm", "text/sgml" },
    { 1150, 250, ".t", "text/troff" },
    { 1151, 250, ".tr", "text/troff" },
    { 1152, 250, ".roff", "text/troff" },
    { 1153, 250, ".man", "text/troff" },
    { 1154, 250, ".me", "text/troff" },
    { 1155, 250, ".ms", "text/troff" },
    { 1170, 260, ".ttl", "text/turtle" },
    { 1180, 270, ".uri", "text/uri-list" },
    { 1181, 270, ".uris", "text/uri-list" },
    { 1182, 270, ".urls", "text/uri-list" },
    { 1190, 280, ".vcard", "text/vcard" },
    { 1191, 280, ".vcf", "text/x-vcard" },
    { 1200, 290, ".curl", "text/vnd.curl" },
    { 1210, 300, ".dcurl", "text/vnd.curl.dcurl" },
    { 1220, 310, ".scurl", "text/vnd.curl.scurl" },
    { 1221, 310, ".mcurl", "text/vnd.curl.mcurl" },
    { 1230, 320, ".sub", "text/vnd.dvb.subtitle" },
    { 1240, 330, ".fly", "text/vnd.fly" },
    { 1250, 340, ".flx", "text/vnd.fmi.flexstor" },
    { 1260, 350, ".gv", "text/vnd.graphviz" },
    { 1270, 360, ".3dml", "text/vnd.in3d.3dml" },
    { 1280, 370, ".spot", "text/vnd.in3d.spot" },
    { 1290, 380, ".jad", "text/vnd.sun.j2me.app-descriptor" },
    { 1300, 390, ".wml", "text/vnd.wap.wml" },
    { 1310, 390, ".wmls", "text/vnd.wap.wmlscript" },
    { 1320, 400, ".asm", "text/x-asm" },
    { 1321, 400, ".s", "text/x-asm" },
    { 1330, 400, ".c", "text/x-c" },
    { 1331, 400, ".cc", "text/x-c" },
    { 1332, 400, ".cxx", "text/x-c" },
    { 1333, 400, ".cpp", "text/x-c" },
    { 1334, 400, ".h", "text/x-c" },
    { 1335, 400, ".hh", "text/x-c" },
    { 1350, 410, ".dic", "text/x-c" },
    { 1360, 420, ".f", "text/x-fortran" },
    { 1361, 420, ".for", "text/x-fortran" },
    { 1362, 420, ".f77", "text/x-fortran" },
    { 1363, 420, ".f90", "text/x-fortran" },
    { 1370, 430, ".java", "text/x-java-source" },
    { 1380, 440, ".opml", "text/x-opml" },
    { 1390, 450, ".p", "text/x-pascal" },
    { 1391, 450, ".pas", "text/x-pascal" },
    { 1400, 460, ".nfo", "text/x-nfo" },
    { 1410, 470, ".etx", "text/x-setext" },
    { 1420, 480, ".sfv", "text/x-sfv" },
    { 1430, 490, ".uu", "text/x-uuencode" },



    { 2000, 1000, ".gif", "image/gif" },

    { 2010, 1010, ".jpg", "image/jpeg" },
    { 2011, 1010, ".jpeg", "image/jpeg" },
    { 2012, 1010, ".jpe", "image/jpeg" },
    { 2013, 1010, ".jfif", "image/jpeg" },
    { 2014, 1010, ".jfi", "image/jpeg" },
    { 2015, 1010, ".jpg", "image/pjpeg" },
    { 2016, 1010, ".jpg", "image/jpg" },

    { 2020, 1020, ".png", "image/png" },
    { 2021, 1020, ".x-png", "image/png" },
    { 2022, 1020, ".png", "image/x-png" },

    { 2030, 1030, ".bmp", "image/bmp" },
    { 2031, 1030, ".bmp", "image/x-bmp" },
    { 2032, 1030, ".bmp", "image/x-windows-bmp" },
    { 2033, 1030, ".bmp", "image/x-win-bitmap" },
    { 2034, 1030, ".bmp", "application/bmp" },
    { 2035, 1030, ".bmp", "application/x-bmp" },
    { 2036, 1030, ".bmp", "application/x-win-bitmap" },

    { 2040, 1030, ".dib", "image/x-ms-bmp" },
    { 2041, 1030, ".rle", "image/x-ms-bmp" },

    { 2050, 1040, ".wbmp", "image/vnd.wap.wbmp" },

    { 2060, 1050, ".xbm", "image/xbm" },
    { 2061, 1050, ".xbm", "image/x-bitmap" },
    { 2062, 1050, ".xbm", "image/x-xbitmap" },
    { 2063, 1050, ".xbm", "image/x-xbm" },

    { 2070, 1060, ".ico", "image/icon" },
    { 2071, 1060, ".ico", "image/x-icon" },

    { 2080, 1070, ".xpm", "image/x-xpixmap" },

    { 2090, 1080, ".tiff", "image/tiff" },
    { 2091, 1080, ".tif", "image/tiff" },

    { 2100, 1090, ".g3", "image/g3fax" },
    { 2110, 1100, ".cgm", "image/cgm" },
    { 2120, 1110, ".ief", "image/ief" },
    { 2130, 1120, ".ktx", "image/ktx" },
    { 2140, 1130, ".btif", "image/prs.btif" },
    { 2150, 1140, ".sgi", "image/sgi" },
    { 2160, 1150, ".svg", "image/svg+xml" },
    { 2170, 1160, ".svgz", "image/svg+xml" },
    { 2180, 1170, ".psd", "image/vnd.adobe.photoshop" },
    { 2190, 1180, ".uvi", "image/vnd.dece.graphic" },
    { 2191, 1180, ".uvvi", "image/vnd.dece.graphic" },
    { 2192, 1180, ".uvg", "image/vnd.dece.graphic" },
    { 2193, 1180, ".uvvg", "image/vnd.dece.graphic" },
    { 2200, 1190, ".sub", "image/vnd.dvb.subtitle" },
    { 2210, 1200, ".djvu", "image/vnd.djvu" },
    { 2210, 1200, ".djv", "image/vnd.djvu" },
    { 2220, 1210, ".dwg", "image/vnd.dwg" },
    { 2230, 1220, ".dxf", "image/vnd.dxf" },
    { 2240, 1230, ".fbs", "image/vnd.fastbidsheet" },
    { 2250, 1240, ".fpx", "image/vnd.fpx" },
    { 2260, 1250, ".fst", "image/vnd.fst" },
    { 2270, 1260, ".mmr", "image/vnd.fujixerox.edmics-mmr" },
    { 2280, 1270, ".rlc", "image/vnd.fujixerox.edmics-rlc" },
    { 2290, 1280, ".mdi", "image/vnd.ms-modi" },
    { 2300, 1290, ".wdp", "image/vnd.ms-photo" },
    { 2310, 1300, ".npx", "image/vnd.net-fpx" },
    { 2320, 1310, ".xif", "image/vnd.xiff" },
    { 2330, 1320, ".webp", "image/webp" },
    { 2340, 1330, ".3ds", "image/x-3ds" },
    { 2350, 1340, ".ras", "image/x-cmu-raster" },
    { 2360, 1350, ".cmx", "image/x-cmx" },
    { 2370, 1360, ".fh", "image/x-freehand" },
    { 2371, 1360, ".fhc", "image/x-freehand" },
    { 2372, 1360, ".fh4", "image/x-freehand" },
    { 2373, 1360, ".fh5", "image/x-freehand" },
    { 2374, 1360, ".fh7", "image/x-freehand" },
    { 2380, 1370, ".sid", "image/x-mrsid-image" },
    { 2390, 1380, ".pcx", "image/x-pcx" },
    { 2391, 1380, ".pcd", "image/pcd" },
    { 2400, 1390, ".pic", "image/x-pict" },
    { 2401, 1390, ".pct", "image/x-pict" },
    { 2402, 1390, ".pict", "image/pict" },
    { 2410, 1400, ".pnm", "image/x-portable-anymap" },
    { 2420, 1410, ".pbm", "image/x-portable-bitmap" },
    { 2430, 1420, ".pgm", "image/x-portable-graymap" },
    { 2440, 1430, ".ppm", "image/x-portable-pixmap" },
    { 2450, 1440, ".rgb", "image/x-rgb" },
    { 2460, 1450, ".tga", "image/x-tga" },
    { 2461, 1450, ".tpic", "image/x-targa" },
    { 2462, 1450, ".vda", "image/x-targa" },
    { 2470, 1460, ".xwd", "image/x-xwindowdump" },
    { 2480, 1470, ".fif", "image/fif" },


    { 3000, 2000, ".adp", "audio/adpcm" },
    { 3010, 2010, ".au", "audio/basic" },
    { 3020, 2020, ".snd", "audio/basic" },
    { 3030, 2030, ".mid", "audio/midi" },
    { 3031, 2030, ".midi", "audio/midi" },
    { 3032, 2030, ".kar", "audio/midi" },
    { 3033, 2030, ".rmi", "audio/midi" },
    { 3040, 2040, ".mp4a", "audio/mp4" },
    { 3050, 2050, ".mpga", "audio/mpeg" },
    { 3051, 2050, ".mp2", "audio/mpeg" },
    { 3052, 2050, ".mp2a", "audio/mpeg" },
    { 3053, 2050, ".mp3", "audio/mpeg" },
    { 3054, 2050, ".m2a", "audio/mpeg" },
    { 3055, 2050, ".m3a", "audio/mpeg" },
    { 3056, 2050, ".m1a", "audio/mpeg" },
    { 3057, 2050, ".mpa", "audio/mpeg" },
    { 3058, 2050, ".mpega", "audio/mpeg" },
    { 3060, 2060, ".oga", "audio/ogg" },
    { 3061, 2060, ".ogg", "audio/ogg" },
    { 3062, 2060, ".spx", "audio/ogg" },
    { 3070, 2070, ".s3m", "audio/s3m" },
    { 3080, 2080, ".sil", "audio/silk" },
    { 3090, 2090, ".uva", "audio/vnd.dece.audio" },
    { 3091, 2090, ".uvva", "audio/vnd.dece.audio" },
    { 3100, 2100, ".eol", "audio/vnd.digital-winds" },
    { 3110, 2110, ".dra", "audio/vnd.dra" },
    { 3120, 2120, ".dts", "audio/vnd.dts" },
    { 3130, 2130, ".dtshd", "audio/vnd.dts.hd" },
    { 3140, 2140, ".lvp", "audio/vnd.lucent.voice" },
    { 3150, 2150, ".pya", "audio/vnd.ms-playready.media.pya" },
    { 3160, 2160, ".ecelp4800", "audio/vnd.nuera.ecelp4800" },
    { 3170, 2170, ".ecelp7470", "audio/vnd.nuera.ecelp7470" },
    { 3180, 2180, ".ecelp9600", "audio/vnd.nuera.ecelp9600" },
    { 3190, 2190, ".rip", "audio/vnd.rip" },
    { 3200, 2200, ".weba", "audio/webm" },
    { 3210, 2210, ".aac", "audio/x-aac" },
    { 3220, 2220, ".aif", "audio/x-aiff" },
    { 3221, 2220, ".aiff", "audio/x-aiff" },
    { 3222, 2220, ".aifc", "audio/x-aiff" },
    { 3230, 2230, ".caf", "audio/x-caf" },
    { 3240, 2240, ".flac", "audio/x-flac" },
    { 3250, 2250, ".mka", "audio/x-matroska" },
    { 3260, 2260, ".m3u", "audio/x-mpegurl" },
    { 3270, 2270, ".wax", "audio/x-ms-wax" },
    { 3280, 2280, ".wma", "audio/x-ms-wma" },
    { 3290, 2290, ".ram", "audio/x-pn-realaudio" },
    { 3300, 2300, ".ra", "audio/x-pn-realaudio" },
    { 3310, 2310, ".rmp", "audio/x-pn-realaudio-plugin" },
    { 3320, 2320, ".wav", "audio/x-wav" },
    { 3330, 2330, ".xm", "audio/xm" },
    { 3340, 2340, ".vqf", "audio/x-twinvq" },
    { 3341, 2340, ".vql", "audio/x-twinvq" },
    { 3342, 2340, ".vqe", "audio/x-twinvq-plugin" },
    { 3400, 2400, ".opus", "audio/x-iqixin-opus" },


    { 4000, 3000, ".3gp", "video/3gpp" },
    { 4010, 3010, ".3g2", "video/3gpp2" },
    { 4020, 3020, ".h261", "video/h261" },
    { 4030, 3030, ".h263", "video/h263" },
    { 4040, 3040, ".h264", "video/h264" },
    { 4050, 3050, ".jpgv", "video/jpeg" },
    { 4060, 3060, ".jpm", "video/jpm" },
    { 4070, 3070, ".jpgm", "video/jpm" },
    { 4080, 3080, ".mj2", "video/mj2" },
    { 4090, 3090, ".mjp2", "video/mj2" },
    { 4100, 3100, ".mp4", "video/mp4" },
    { 4101, 3100, ".mp4v", "video/mp4" },
    { 4102, 3100, ".mpg4", "video/mp4" },
    { 4110, 3110, ".mpeg", "video/mpeg" },
    { 4111, 3110, ".mpg", "video/mpeg" },
    { 4112, 3110, ".mpe", "video/mpeg" },
    { 4113, 3110, ".m1v", "video/mpeg" },
    { 4114, 3110, ".m2v", "video/mpeg" },
    { 4115, 3110, ".m1s", "video/mpeg" },
    { 4116, 3110, ".m2s", "video/mpeg" },
    { 4117, 3110, ".mpv", "video/mpeg" },
    { 4120, 3120, ".ogv", "video/ogg" },
    { 4130, 3130, ".qt", "video/quicktime" },
    { 4131, 3130, ".mov", "video/quicktime" },
    { 4132, 3130, ".moov", "video/quicktime" },
    { 4140, 3140, ".uvh", "video/vnd.dece.hd" },
    { 4141, 3140, ".uvvh", "video/vnd.dece.hd" },
    { 4150, 3150, ".uvm", "video/vnd.dece.mobile" },
    { 4151, 3150, ".uvvm", "video/vnd.dece.mobile" },
    { 4160, 3160, ".uvp", "video/vnd.dece.pd" },
    { 4170, 3170, ".uvvp", "video/vnd.dece.pd" },
    { 4180, 3180, ".uvs", "video/vnd.dece.sd" },
    { 4181, 3180, ".uvvs", "video/vnd.dece.sd" },
    { 4190, 3190, ".uvv", "video/vnd.dece.video" },
    { 4191, 3190, ".uvvv", "video/vnd.dece.video" },
    { 4200, 3200, ".dvb", "video/vnd.dvb.file" },
    { 4210, 3210, ".fvt", "video/vnd.fvt" },
    { 4220, 3220, ".mxu", "video/vnd.mpegurl" },
    { 4221, 3220, ".m4u", "video/vnd.mpegurl" },
    { 4230, 3230, ".pyv", "video/vnd.ms-playready.media.pyv" },
    { 4240, 3240, ".uvu", "video/vnd.uvvu.mp4" },
    { 4241, 3240, ".uvvu", "video/vnd.uvvu.mp4" },
    { 4250, 3250, ".viv", "video/vnd.vivo" },
    { 4260, 3260, ".webm", "video/webm" },
    { 4270, 3270, ".f4v", "video/x-f4v" },
    { 4280, 3280, ".fli", "video/x-fli" },
    { 4290, 3290, ".flv", "video/x-flv" },
    { 4295, 3295, ".flc", "video/flc" },
    { 4300, 3300, ".m4v", "video/x-m4v" },
    { 4310, 3310, ".mkv", "video/x-matroska" },
    { 4311, 3310, ".mk3d", "video/x-matroska" },
    { 4320, 3320, ".mks", "video/x-matroska" },
    { 4330, 3330, ".mng", "video/x-mng" },
    { 4340, 3340, ".asf", "video/x-ms-asf" },
    { 4341, 3340, ".asx", "video/x-ms-asf" },
    { 4350, 3350, ".vob", "video/x-ms-vob" },
    { 4360, 3360, ".wm", "video/x-ms-wm" },
    { 4370, 3370, ".wmv", "video/x-ms-wmv" },
    { 4380, 3380, ".wmx", "video/x-ms-wmx" },
    { 4390, 3390, ".wvx", "video/x-ms-wvx" },
    { 4400, 3400, ".avi", "video/x-msvideo" },
    { 4410, 3410, ".movie", "video/x-sgi-movie" },
    { 4420, 3420, ".smv", "video/x-smv" },
    { 4430, 3430, ".ice", "x-conference/x-cooltalk" },
    { 4440, 3440, ".swf", "application/x-shockwave-flash" },
    { 4450, 3450, ".rv", "application/vnd.rn-realmedia" },
    { 4460, 3460, ".rmvb", "application/vnd.rn-realmedia-vbr" },
    { 4470, 3470, ".rm", "application/vnd.rn-realmedia" },
    { 4480, 3480, ".vdo", "video/vdo" },
    { 4490, 3490, ".vivo", "video/vivo" },

    { 4500, 3500, ".m3u8", "application/x-mpegURL" },
    { 4510, 3510, ".ts", "video/MP2T" },



    { 5000, 4000, ".xml", "application/xml" },
    { 5001, 4000, ".xsl", "application/xml" },
    { 5002, 4001, ".dsml", "application/dsml" },
    { 5010, 4010, ".dtd", "application/xml-dtd" },
    { 5020, 4020, ".xhtml", "application/xhtml+xml" },
    { 5021, 4020, ".xht", "application/xhtml+xml" },
    { 5030, 4030, ".xop", "application/xop+xml" },
    { 5040, 4040, ".xpl", "application/xproc+xml" },
    { 5050, 4050, ".xslt", "application/xslt+xml" },
    { 5060, 4060, ".xspf", "application/xspf+xml" },
    { 5070, 4070, ".mxml", "application/xv+xml" },
    { 5071, 4070, ".xhvml", "application/xv+xml" },
    { 5072, 4070, ".xvml", "application/xv+xml" },
    { 5073, 4070, ".xvm", "application/xv+xml" },
    { 5080, 4080, ".yin", "application/yin+xml" },
    { 5090, 4090, ".xdf", "application/xcap-diff+xml" },
    { 5100, 4100, ".xenc", "application/xenc+xml" },

    { 5110, 4110, ".lostxml", "application/lost+xml" },
    { 5120, 4120, ".vxml", "application/voicexml+xml" },
    { 5130, 4130, ".rss", "application/rss+xml" },
    { 5140, 4140, ".smi", "application/smil+xml" },
    { 5141, 4140, ".smil", "application/smil+xml" },
    { 5150, 4150, ".wbxml", "application/vnd.wap.wbxml" },
    { 5160, 4160, ".wmlc", "application/vnd.wap.wmlc" },
    { 5170, 4170, ".wmlsc", "application/vnd.wap.wmlscriptc" },
    { 5180, 4180, ".xsm", "application/vnd.syncml+xml" },
    { 5190, 4190, ".bdm", "application/vnd.syncml.dm+wbxml" },
    { 5200, 4200, ".xdm", "application/vnd.syncml.dm+xml" },
    { 5210, 4210, ".hlp", "application/winhlp" },
    { 5220, 4220, ".wsdl", "application/wsdl+xml" },
    { 5230, 4230, ".wspolicy", "application/wspolicy+xml" },
    { 5240, 4240, ".si", "text/vnd.wap.si" },
    { 5241, 4240, ".sic", "application/vnd.wap.sic" },
    { 5250, 4250, ".sl", "text/vnd.wap.sl" },
    { 5251, 4250, ".slc", "application/vnd.wap.slc" },
    { 5260, 4260, ".co", "text/vnd.wap.co" },
    { 5261, 4260, ".coc", "application/vnd.wap.coc" },
    { 5270, 4270, ".mms", "application/vnd.wap.mms-message" },


    { 5400, 4400, ".jar", "application/java-archive" },
    { 5410, 4410, ".ser", "application/java-serialized-object" },
    { 5420, 4420, ".class", "application/java-vm" },
    { 5430, 4430, ".jnlp", "application/x-java-jnlp-file" },
    { 5440, 4440, ".rms", "application/vnd.jcp.javame.midlet-rms" },
    { 5450, 4450, ".js", "application/javascript" },
    { 5460, 4460, ".json", "application/json" },
    { 5470, 4470, ".jsonml", "application/jsonml+json" },


    { 6000, 5000, ".pdf", "application/pdf" },

    { 6100, 5100, ".doc", "application/msword" },
    { 6110, 5110, ".dot", "application/msword" },
    { 6120, 5120, ".rtf", "application/rtf" },

    { 6130, 5130, ".xls", "application/vnd.ms-excel" },
    { 6131, 5130, ".xlm", "application/vnd.ms-excel" },
    { 6132, 5130, ".xla", "application/vnd.ms-excel" },
    { 6133, 5130, ".xlc", "application/vnd.ms-excel" },
    { 6134, 5130, ".xlt", "application/vnd.ms-excel" },
    { 6135, 5130, ".xlw", "application/vnd.ms-excel" },

    { 6140, 5140, ".xlam", "application/vnd.ms-excel.addin.macroenabled.12" },
    { 6141, 5140, ".xlsb", "application/vnd.ms-excel.sheet.binary.macroenabled.12" },
    { 6142, 5140, ".xlsm", "application/vnd.ms-excel.sheet.macroenabled.12" },
    { 6143, 5140, ".xltm", "application/vnd.ms-excel.template.macroenabled.12" },

    { 6150, 5150, ".vsd", "application/vnd.visio" },
    { 6151, 5150, ".vst", "application/vnd.visio" },
    { 6152, 5150, ".vss", "application/vnd.visio" },
    { 6153, 5150, ".vsw", "application/vnd.visio" },

    { 6160, 5160, ".ppt", "application/vnd.ms-powerpoint" },
    { 6161, 5160, ".pps", "application/vnd.ms-powerpoint" },
    { 6162, 5160, ".pot", "application/vnd.ms-powerpoint" },
    { 6163, 5160, ".ppam", "application/vnd.ms-powerpoint.addin.macroenabled.12" },
    { 6164, 5160, ".pptm", "application/vnd.ms-powerpoint.presentation.macroenabled.12" },
    { 6165, 5160, ".sldm", "application/vnd.ms-powerpoint.slide.macroenabled.12" },
    { 6166, 5160, ".ppsm", "application/vnd.ms-powerpoint.slideshow.macroenabled.12" },
    { 6167, 5160, ".potm", "application/vnd.ms-powerpoint.template.macroenabled.12" },

    { 6170, 5170, ".mpp", "application/vnd.ms-project" },
    { 6171, 5170, ".mpt", "application/vnd.ms-project" },

    { 6180, 5180, ".pptx", "application/vnd.openxmlformats-officedocument.presentationml.presentation" },
    { 6190, 5190, ".sldx", "application/vnd.openxmlformats-officedocument.presentationml.slide" },
    { 6200, 5200, ".ppsx", "application/vnd.openxmlformats-officedocument.presentationml.slideshow" },
    { 6210, 5210, ".potx", "application/vnd.openxmlformats-officedocument.presentationml.template" },
    { 6230, 5230, ".xlsx", "application/vnd.openxmlformats-officedocument.spreadsheetml.sheet" },
    { 6240, 5240, ".xltx", "application/vnd.openxmlformats-officedocument.spreadsheetml.template" },
    { 6250, 5250, ".docx", "application/vnd.openxmlformats-officedocument.wordprocessingml.document" },
    { 6260, 5260, ".dotx", "application/vnd.openxmlformats-officedocument.wordprocessingml.template" },

    { 6270, 5270, ".docm", "application/vnd.ms-word.document.macroenabled.12" },
    { 6280, 5280, ".dotm", "application/vnd.ms-word.template.macroenabled.12" },
    { 6290, 5290, ".wps", "application/vnd.ms-works" },
    { 6291, 5290, ".wks", "application/vnd.ms-works" },
    { 6292, 5290, ".wcm", "application/vnd.ms-works" },
    { 6293, 5290, ".wdb", "application/vnd.ms-works" },
    { 6300, 5300, ".wpl", "application/vnd.ms-wpl" },
    { 6310, 5310, ".xps", "application/vnd.ms-xpsdocument" },

    { 6400, 5400, ".eot", "application/vnd.ms-fontobject" },
    { 6410, 5410, ".chm", "application/vnd.ms-htmlhelp" },
    { 6420, 5420, ".ims", "application/vnd.ms-ims" },
    { 6430, 5430, ".lrm", "application/vnd.ms-lrm" },
    { 6440, 5440, ".thmx", "application/vnd.ms-officetheme" },
    { 6450, 5450, ".cat", "application/vnd.ms-pki.seccat" },
    { 6460, 5460, ".stl", "application/vnd.ms-pki.stl" },

    { 6470, 5470, ".cil", "application/vnd.ms-artgalry" },
    { 6480, 5480, ".cab", "application/vnd.ms-cab-compressed" },

    { 6490, 5490, ".application", "application/x-ms-application" },
    { 6500, 5500, ".lnk", "application/x-ms-shortcut" },
    { 6510, 5510, ".wmd", "application/x-ms-wmd" },
    { 6520, 5520, ".wmz", "application/x-ms-wmz" },
    { 6530, 5530, ".xbap", "application/x-ms-xbap" },
    { 6540, 5540, ".mdb", "application/x-msaccess" },
    { 6550, 5550, ".obd", "application/x-msbinder" },
    { 6560, 5560, ".crd", "application/x-mscardfile" },
    { 6570, 5570, ".clp", "application/x-msclip" },
    { 6580, 5580, ".exe", "application/x-msdownload" },
    { 6581, 5580, ".dll", "application/x-msdownload" },
    { 6582, 5580, ".com", "application/x-msdownload" },
    { 6583, 5580, ".bat", "application/x-msdownload" },
    { 6584, 5580, ".msi", "application/x-msdownload" },
    { 6590, 5590, ".mvb", "application/x-msmediaview" },
    { 6591, 5590, ".m13", "application/x-msmediaview" },
    { 6592, 5590, ".m14", "application/x-msmediaview" },
    { 6600, 5600, ".wmf", "application/x-msmetafile" },
    { 6601, 5601, ".wmz", "application/x-msmetafile" },
    { 6602, 5602, ".emf", "application/x-msmetafile" },
    { 6603, 5603, ".emz", "application/x-msmetafile" },
    { 6610, 5610, ".mny", "application/x-msmoney" },
    { 6620, 5620, ".pub", "application/x-mspublisher" },
    { 6630, 5630, ".scd", "application/x-msschedule" },
    { 6640, 5640, ".trm", "application/x-msterminal" },
    { 6650, 5650, ".wri", "application/x-mswrite" },

    { 6800, 5800, ".odc", "application/vnd.oasis.opendocument.chart" },
    { 6810, 5810, ".otc", "application/vnd.oasis.opendocument.chart-template" },
    { 6820, 5820, ".odb", "application/vnd.oasis.opendocument.database" },
    { 6830, 5830, ".odf", "application/vnd.oasis.opendocument.formula" },
    { 6840, 5840, ".odft", "application/vnd.oasis.opendocument.formula-template" },
    { 6850, 5850, ".odg", "application/vnd.oasis.opendocument.graphics" },
    { 6860, 5860, ".otg", "application/vnd.oasis.opendocument.graphics-template" },
    { 6870, 5870, ".odi", "application/vnd.oasis.opendocument.image" },
    { 6880, 5880, ".oti", "application/vnd.oasis.opendocument.image-template" },
    { 6890, 5890, ".odp", "application/vnd.oasis.opendocument.presentation" },
    { 6900, 5900, ".otp", "application/vnd.oasis.opendocument.presentation-template" },
    { 6910, 5910, ".ods", "application/vnd.oasis.opendocument.spreadsheet" },
    { 6920, 5920, ".ots", "application/vnd.oasis.opendocument.spreadsheet-template" },
    { 6930, 5930, ".odt", "application/vnd.oasis.opendocument.text" },
    { 6940, 5940, ".odm", "application/vnd.oasis.opendocument.text-master" },
    { 6950, 5950, ".ott", "application/vnd.oasis.opendocument.text-template" },
    { 6960, 5960, ".oth", "application/vnd.oasis.opendocument.text-web" },
    { 6970, 5970, ".oxt", "application/vnd.openofficeorg.extension" },

    { 7100, 6100, ".123", "application/vnd.lotus-1-2-3" },
    { 7110, 6110, ".apr", "application/vnd.lotus-approach" },
    { 7120, 6120, ".pre", "application/vnd.lotus-freelance" },
    { 7130, 6130, ".nsf", "application/vnd.lotus-notes" },
    { 7140, 6140, ".org", "application/vnd.lotus-organizer" },
    { 7150, 6150, ".scm", "application/vnd.lotus-screencam" },
    { 7160, 6160, ".lwp", "application/vnd.lotus-wordpro" },

    { 7200, 6200, ".ai", "application/postscript" },
    { 7201, 6200, ".eps", "application/postscript" },
    { 7202, 6200, ".ps", "application/postscript" },
    { 7203, 6200, ".epsf", "application/postscript" },



    { 8000, 7000, ".sis", "application/vnd.symbian.install" },
    { 8001, 7000, ".sisx", "application/vnd.symbian.install" },
    { 8010, 7010, ".n-gage", "application/vnd.nokia.n-gage.symbian.install" },
    { 8020, 7020, ".air", "application/vnd.adobe.air-application-installer-package+zip" },
    { 8030, 7030, ".mpkg", "application/vnd.apple.installer+xml" },
    { 8040, 7040, ".apk", "application/vnd.android.package-archive" },
    { 8050, 7050, ".install", "application/x-install-instructions" },
    { 8060, 7060, ".xpi", "application/x-xpinstall" },
    { 8070, 7070, ".crx", "application/x-chrome-extension" },


    { 8500, 7500, ".zip", "application/zip" },
    { 8510, 7510, ".bz", "application/x-bzip" },
    { 8520, 7520, ".bz2", "application/x-bzip2" },
    { 8521, 7520, ".boz", "application/x-bzip2" },
    { 8530, 7530, ".z", "application/x-compress" },
    { 8540, 7540, ".gz", "application/gzip" },
    { 8550, 7550, ".tgz", "application/gzip" },
    { 8551, 7550, ".tgz", "application/x-gzip" },
    { 8560, 7560, ".tar", "application/x-tar" },
    { 8570, 7570, ".rar", "application/x-rar-compressed" },
    { 8580, 7580, ".lzh", "application/x-lzh-compressed" },
    { 8581, 7580, ".lha", "application/x-lzh-compressed" },
    { 8590, 7590, ".7z", "application/x-7z-compressed" },
    { 8600, 7600, ".ace", "application/x-ace-compressed" },
    { 8610, 7610, ".gca", "application/x-gca-compressed" },
    { 8620, 7620, ".cfs", "application/x-cfs-compressed" },
    { 8630, 7630, ".dgc", "application/x-dgc-compressed" },
    { 8640, 7640, ".ustar", "application/x-ustar" },
    { 8650, 7650, ".gtar", "application/x-gtar" },
    { 8660, 7660, ".xz", "application/x-xz" },


    { 30010, 30010, ".ez", "application/andrew-inset" },
    { 30020, 30020, ".aw", "application/applixware" },
    { 30030, 30030, ".atom", "application/atom+xml" },
    { 30040, 30040, ".atomcat", "application/atomcat+xml" },
    { 30050, 30050, ".atomsvc", "application/atomsvc+xml" },
    { 30060, 30060, ".ccxml", "application/ccxml+xml" },
    { 30070, 30070, ".cdmia", "application/cdmi-capability" },
    { 30080, 30080, ".cdmic", "application/cdmi-container" },
    { 30090, 30090, ".cdmid", "application/cdmi-domain" },
    { 30100, 30100, ".cdmio", "application/cdmi-object" },
    { 30110, 30110, ".cdmiq", "application/cdmi-queue" },
    { 30120, 30120, ".cu", "application/cu-seeme" },
    { 30130, 30130, ".davmount", "application/davmount+xml" },
    { 30140, 30140, ".dbk", "application/docbook+xml" },
    { 30150, 30150, ".dssc", "application/dssc+der" },
    { 30160, 30160, ".xdssc", "application/dssc+xml" },
    { 30170, 30170, ".ecma", "application/ecmascript" },
    { 30180, 30180, ".emma", "application/emma+xml" },
    { 30190, 30190, ".epub", "application/epub+zip" },
    { 30200, 30200, ".exi", "application/exi" },
    { 30210, 30210, ".pfr", "application/font-tdpfr" },
    { 30220, 30220, ".gml", "application/gml+xml" },
    { 30230, 30230, ".gpx", "application/gpx+xml" },
    { 30240, 30240, ".gxf", "application/gxf" },
    { 30250, 30250, ".stk", "application/hyperstudio" },
    { 30260, 30260, ".ink", "application/inkml+xml" },
    { 30261, 30260, ".inkml", "application/inkml+xml" },
    { 30270, 30270, ".ipfix", "application/ipfix" },
    { 30280, 30280, ".hqx", "application/mac-binhex40" },

    { 30290, 30290, ".cpt", "application/mac-compactpro" },
    { 30300, 30300, ".mads", "application/mads+xml" },
    { 30310, 30310, ".mrc", "application/marc" },
    { 30320, 30320, ".mrcx", "application/marcxml+xml" },
    { 30330, 30330, ".ma", "application/mathematica" },
    { 30331, 30330, ".nb", "application/mathematica" },
    { 30332, 30330, ".mb", "application/mathematica" },
    { 30340, 30340, ".mathml", "application/mathml+xml" },
    { 30350, 30350, ".mbox", "application/mbox" },
    { 30360, 30360, ".mscml", "application/mediaservercontrol+xml" },
    { 30370, 30370, ".metalink", "application/metalink+xml" },
    { 30380, 30380, ".meta4", "application/metalink4+xml" },
    { 30390, 30390, ".mets", "application/mets+xml" },
    { 30400, 30400, ".mods", "application/mods+xml" },
    { 30410, 30410, ".m21", "application/mp21" },
    { 30411, 30410, ".mp21", "application/mp21" },
    { 30420, 30420, ".mp4s", "application/mp4" },

    { 30430, 30430, ".mxf", "application/mxf" },

    { 30440, 30440, ".bin", "application/octet-stream" },
    { 30441, 30440, ".dms", "application/octet-stream" },
    { 30442, 30440, ".lrf", "application/octet-stream" },
    { 30443, 30440, ".mar", "application/octet-stream" },
    { 30444, 30440, ".so", "application/octet-stream" },
    { 30445, 30440, ".dist", "application/octet-stream" },
    { 30446, 30440, ".distz", "application/octet-stream" },
    { 30447, 30440, ".pkg", "application/octet-stream" },
    { 30448, 30440, ".bpk", "application/octet-stream" },
    { 30449, 30440, ".dump", "application/octet-stream" },
    { 30450, 30440, ".elc", "application/octet-stream" },
    { 30451, 30440, ".deploy", "application/octet-stream" },

    { 30470, 30470, ".oda", "application/oda" },
    { 30480, 30480, ".opf", "application/oebps-package+xml" },
    { 30490, 30490, ".ogx", "application/ogg" },
    { 30500, 30500, ".omdoc", "application/omdoc+xml" },
    { 30510, 30510, ".onetoc", "application/onenote" },
    { 30511, 30510, ".onetoc2", "application/onenote" },
    { 30512, 30510, ".onetmp", "application/onenote" },
    { 30513, 30510, ".onepkg", "application/onenote" },
    { 30520, 30520, ".oxps", "application/oxps" },
    { 30530, 30530, ".xer", "application/patch-ops-error+xml" },

    { 30540, 30540, ".pgp", "application/pgp-encrypted" },
    { 30550, 30550, ".asc", "application/pgp-signature" },
    { 30551, 30550, ".sig", "application/pgp-signature" },
    { 30560, 30560, ".prf", "application/pics-rules" },
    { 30570, 30570, ".p10", "application/pkcs10" },
    { 30580, 30580, ".p7m", "application/pkcs7-mime" },
    { 30581, 30580, ".p7c", "application/pkcs7-mime" },
    { 30590, 30590, ".p7s", "application/pkcs7-signature" },
    { 30600, 30600, ".p8", "application/pkcs8" },
    { 30610, 30610, ".ac", "application/pkix-attr-cert" },
    { 30620, 30620, ".cer", "application/pkix-cert" },
    { 30630, 30630, ".crl", "application/pkix-crl" },
    { 30640, 30640, ".pkipath", "application/pkix-pkipath" },
    { 30650, 30650, ".pki", "application/pkixcmp" },
    { 30660, 30660, ".pls", "application/pls+xml" },

    { 30670, 30670, ".cww", "application/prs.cww" },
    { 30680, 30680, ".pskcxml", "application/pskc+xml" },
    { 30690, 30690, ".rdf", "application/rdf+xml" },
    { 30700, 30700, ".rif", "application/reginfo+xml" },
    { 30710, 30710, ".rnc", "application/relax-ng-compact-syntax" },
    { 30720, 30720, ".rl", "application/resource-lists+xml" },
    { 30730, 30730, ".rld", "application/resource-lists-diff+xml" },
    { 30740, 30740, ".rs", "application/rls-services+xml" },
    { 30750, 30750, ".gbr", "application/rpki-ghostbusters" },
    { 30760, 30760, ".mft", "application/rpki-manifest" },
    { 30770, 30770, ".roa", "application/rpki-roa" },
    { 30780, 30780, ".rsd", "application/rsd+xml" },

    { 30790, 30790, ".sbml", "application/sbml+xml" },
    { 30800, 30800, ".scq", "application/scvp-cv-request" },
    { 30810, 30810, ".scs", "application/scvp-cv-response" },
    { 30820, 30820, ".spq", "application/scvp-vp-request" },
    { 30830, 30830, ".spp", "application/scvp-vp-response" },
    { 30840, 30840, ".sdp", "application/sdp" },
    { 30850, 30850, ".setpay", "application/set-payment-initiation" },
    { 30860, 30860, ".setreg", "application/set-registration-initiation" },
    { 30870, 30870, ".shf", "application/shf+xml" },

    { 30880, 30880, ".rq", "application/sparql-query" },
    { 30890, 30890, ".srx", "application/sparql-results+xml" },
    { 30900, 30900, ".gram", "application/srgs" },
    { 39010, 30910, ".grxml", "application/srgs+xml" },
    { 30920, 30920, ".sru", "application/sru+xml" },
    { 30930, 30930, ".ssdl", "application/ssdl+xml" },
    { 30940, 30940, ".ssml", "application/ssml+xml" },
    { 30950, 30950, ".tei", "application/tei+xml" },
    { 30960, 30960, ".teicorpus", "application/tei+xml" },
    { 30970, 30970, ".tfi", "application/thraud+xml" },
    { 30980, 30980, ".tsd", "application/timestamped-data" },
    { 30990, 30990, ".plb", "application/vnd.3gpp.pic-bw-large" },
    { 31000, 31000, ".psb", "application/vnd.3gpp.pic-bw-small" },
    { 31010, 31010, ".pvb", "application/vnd.3gpp.pic-bw-var" },
    { 31020, 31020, ".tcap", "application/vnd.3gpp2.tcap" },
    { 31030, 31030, ".pwn", "application/vnd.3m.post-it-notes" },
    { 31040, 31040, ".aso", "application/vnd.accpac.simply.aso" },
    { 31050, 31050, ".imp", "application/vnd.accpac.simply.imp" },
    { 31060, 31060, ".acu", "application/vnd.acucobol" },
    { 31070, 31070, ".atc", "application/vnd.acucorp" },
    { 31071, 31070, ".acutc", "application/vnd.acucorp" },
    { 31080, 31080, ".fcdt", "application/vnd.adobe.formscentral.fcdt" },
    { 31090, 31090, ".fxp", "application/vnd.adobe.fxp" },
    { 31191, 31090, ".fxpl", "application/vnd.adobe.fxp" },
    { 31100, 31100, ".xdp", "application/vnd.adobe.xdp+xml" },
    { 31110, 31110, ".xfdf", "application/vnd.adobe.xfdf" },
    { 31120, 31120, ".ahead", "application/vnd.ahead.space" },
    { 31130, 31130, ".azf", "application/vnd.airzip.filesecure.azf" },
    { 31140, 31140, ".azs", "application/vnd.airzip.filesecure.azs" },
    { 31150, 31150, ".azw", "application/vnd.amazon.ebook" },
    { 31160, 31160, ".acc", "application/vnd.americandynamics.acc" },
    { 31170, 31170, ".ami", "application/vnd.amiga.ami" },
    { 31180, 31180, ".cii", "application/vnd.anser-web-certificate-issue-initiation" },
    { 31190, 31190, ".fti", "application/vnd.anser-web-funds-transfer-initiation" },
    { 31200, 31200, ".atx", "application/vnd.antix.game-component" },
    { 31210, 31210, ".m3u8", "application/vnd.apple.mpegurl" },
    { 31220, 31220, ".swi", "application/vnd.aristanetworks.swi" },
    { 31230, 31230, ".iota", "application/vnd.astraea-software.iota" },
    { 31240, 31240, ".aep", "application/vnd.audiograph" },
    { 31250, 31250, ".mpm", "application/vnd.blueice.multipass" },
    { 31260, 31260, ".bmi", "application/vnd.bmi" },
    { 31270, 31270, ".rep", "application/vnd.businessobjects" },
    { 31280, 31280, ".cdxml", "application/vnd.chemdraw+xml" },
    { 31290, 31290, ".mmd", "application/vnd.chipnuts.karaoke-mmd" },
    { 31300, 31300, ".cdy", "application/vnd.cinderella" },
    { 31310, 31310, ".cla", "application/vnd.claymore" },
    { 31320, 31320, ".rp9", "application/vnd.cloanto.rp9" },
    { 31330, 31330, ".c4g", "application/vnd.clonk.c4group" },
    { 31331, 31330, ".c4d", "application/vnd.clonk.c4group" },
    { 31332, 31330, ".c4f", "application/vnd.clonk.c4group" },
    { 31333, 31330, ".c4p", "application/vnd.clonk.c4group" },
    { 31334, 31330, ".c4u", "application/vnd.clonk.c4group" },
    { 31340, 31340, ".c11amc", "application/vnd.cluetrust.cartomobile-config" },
    { 31350, 31350, ".c11amz", "application/vnd.cluetrust.cartomobile-config-pkg" },
    { 31360, 31360, ".csp", "application/vnd.commonspace" },
    { 31370, 31370, ".cdbcmsg", "application/vnd.contact.cmsg" },
    { 31380, 31380, ".cmc", "application/vnd.cosmocaller" },
    { 31390, 31390, ".clkx", "application/vnd.crick.clicker" },
    { 31400, 31400, ".clkk", "application/vnd.crick.clicker.keyboard" },
    { 31410, 31410, ".clkp", "application/vnd.crick.clicker.palette" },
    { 31420, 31420, ".clkt", "application/vnd.crick.clicker.template" },
    { 31430, 31430, ".clkw", "application/vnd.crick.clicker.wordbank" },
    { 31440, 31440, ".wbs", "application/vnd.criticaltools.wbs+xml" },
    { 31450, 31450, ".pml", "application/vnd.ctc-posml" },
    { 31460, 31460, ".ppd", "application/vnd.cups-ppd" },
    { 31470, 31470, ".car", "application/vnd.curl.car" },
    { 31480, 31480, ".pcurl", "application/vnd.curl.pcurl" },
    { 31490, 31490, ".dart", "application/vnd.dart" },
    { 31500, 31500, ".rdz", "application/vnd.data-vision.rdz" },
    { 31510, 31510, ".uvf", "application/vnd.dece.data" },
    { 31511, 31510, ".uvvf", "application/vnd.dece.data" },
    { 31512, 31510, ".uvd", "application/vnd.dece.data" },
    { 31513, 31510, ".uvvd", "application/vnd.dece.data" },
    { 31520, 31520, ".uvt", "application/vnd.dece.ttml+xml" },
    { 31521, 31520, ".uvvt", "application/vnd.dece.ttml+xml" },
    { 31530, 31530, ".uvx", "application/vnd.dece.unspecified" },
    { 31531, 31530, ".uvvx", "application/vnd.dece.unspecified" },
    { 31540, 31540, ".uvz", "application/vnd.dece.zip" },
    { 31541, 31540, ".uvvz", "application/vnd.dece.zip" },
    { 31550, 31550, ".fe_launch", "application/vnd.denovo.fcselayout-link" },
    { 31560, 31560, ".dna", "application/vnd.dna" },
    { 31570, 31570, ".mlp", "application/vnd.dolby.mlp" },
    { 31580, 31580, ".dpg", "application/vnd.dpgraph" },
    { 31590, 31590, ".dfac", "application/vnd.dreamfactory" },
    { 31600, 31600, ".kpxx", "application/vnd.ds-keypoint" },
    { 31610, 31610, ".ait", "application/vnd.dvb.ait" },
    { 31620, 31620, ".svc", "application/vnd.dvb.service" },
    { 31630, 31630, ".geo", "application/vnd.dynageo" },
    { 31640, 31640, ".mag", "application/vnd.ecowin.chart" },
    { 31650, 31650, ".nml", "application/vnd.enliven" },
    { 31660, 31660, ".esf", "application/vnd.epson.esf" },
    { 31670, 31670, ".msf", "application/vnd.epson.msf" },
    { 31680, 31680, ".qam", "application/vnd.epson.quickanime" },
    { 31690, 31690, ".slt", "application/vnd.epson.salt" },
    { 31700, 31700, ".ssf", "application/vnd.epson.ssf" },
    { 31710, 31710, ".es3", "application/vnd.eszigno3+xml" },
    { 31711, 31710, ".et3", "application/vnd.eszigno3+xml" },
    { 31720, 31720, ".ez2", "application/vnd.ezpix-album" },
    { 31730, 31730, ".ez3", "application/vnd.ezpix-package" },
    { 31740, 31740, ".fdf", "application/vnd.fdf" },
    { 31750, 31750, ".mseed", "application/vnd.fdsn.mseed" },
    { 31760, 31760, ".seed", "application/vnd.fdsn.seed" },
    { 31761, 31760, ".dataless", "application/vnd.fdsn.seed" },
    { 31770, 31770, ".gph", "application/vnd.flographit" },
    { 31780, 31780, ".ftc", "application/vnd.fluxtime.clip" },
    { 31790, 31790, ".fm", "application/vnd.framemaker" },
    { 31791, 31790, ".frame", "application/vnd.framemaker" },
    { 31792, 31790, ".maker", "application/vnd.framemaker" },
    { 31793, 31790, ".book", "application/vnd.framemaker" },
    { 31800, 31800, ".fnc", "application/vnd.frogans.fnc" },
    { 31810, 31810, ".ltf", "application/vnd.frogans.ltf" },
    { 31820, 31820, ".fsc", "application/vnd.fsc.weblaunch" },
    { 31830, 31830, ".oas", "application/vnd.fujitsu.oasys" },
    { 31840, 31840, ".oa2", "application/vnd.fujitsu.oasys2" },
    { 31850, 31850, ".oa3", "application/vnd.fujitsu.oasys3" },
    { 31860, 31860, ".fg5", "application/vnd.fujitsu.oasysgp" },
    { 31870, 31870, ".bh2", "application/vnd.fujitsu.oasysprs" },
    { 31880, 31880, ".ddd", "application/vnd.fujixerox.ddd" },
    { 31890, 31890, ".xdw", "application/vnd.fujixerox.docuworks" },
    { 31900, 31900, ".xbd", "application/vnd.fujixerox.docuworks.binder" },
    { 31910, 31910, ".fzs", "application/vnd.fuzzysheet" },
    { 31920, 31920, ".txd", "application/vnd.genomatix.tuxedo" },
    { 31930, 31930, ".ggb", "application/vnd.geogebra.file" },
    { 31940, 31940, ".ggt", "application/vnd.geogebra.tool" },
    { 31950, 31950, ".gex", "application/vnd.geometry-explorer" },
    { 31951, 31950, ".gre", "application/vnd.geometry-explorer" },
    { 31960, 31960, ".gxt", "application/vnd.geonext" },
    { 31970, 31970, ".g2w", "application/vnd.geoplan" },
    { 31980, 31980, ".g3w", "application/vnd.geospace" },
    { 31990, 31990, ".gmx", "application/vnd.gmx" },
    { 32000, 32000, ".kml", "application/vnd.google-earth.kml+xml" },
    { 32010, 32010, ".kmz", "application/vnd.google-earth.kmz" },
    { 32020, 32020, ".gqf", "application/vnd.grafeq" },
    { 32021, 32020, ".gqs", "application/vnd.grafeq" },
    { 32030, 32030, ".gac", "application/vnd.groove-account" },
    { 32040, 32040, ".ghf", "application/vnd.groove-help" },
    { 32050, 32050, ".gim", "application/vnd.groove-identity-message" },
    { 32060, 32060, ".grv", "application/vnd.groove-injector" },
    { 32070, 32070, ".gtm", "application/vnd.groove-tool-message" },
    { 32080, 32080, ".tpl", "application/vnd.groove-tool-template" },
    { 32090, 32090, ".vcg", "application/vnd.groove-vcard" },
    { 32100, 32100, ".hal", "application/vnd.hal+xml" },
    { 32110, 32110, ".zmm", "application/vnd.handheld-entertainment+xml" },
    { 32120, 32120, ".hbci", "application/vnd.hbci" },
    { 32130, 32130, ".les", "application/vnd.hhe.lesson-player" },
    { 32140, 32140, ".hpgl", "application/vnd.hp-hpgl" },
    { 32141, 32140, ".hpid", "application/vnd.hp-hpid" },
    { 32150, 32150, ".hps", "application/vnd.hp-hps" },
    { 32160, 32160, ".jlt", "application/vnd.hp-jlyt" },
    { 32170, 32170, ".pcl", "application/vnd.hp-pcl" },
    { 32180, 32180, ".pclxl", "application/vnd.hp-pclxl" },
    { 32190, 32190, ".sfd-hdstx", "application/vnd.hydrostatix.sof-data" },
    { 32200, 32200, ".mpy", "application/vnd.ibm.minipay" },
    { 32210, 32210, ".afp", "application/vnd.ibm.modcap" },
    { 32220, 32220, ".listafp", "application/vnd.ibm.modcap" },
    { 32230, 32230, ".list3820", "application/vnd.ibm.modcap" },
    { 32240, 32240, ".irm", "application/vnd.ibm.rights-management" },
    { 32250, 32250, ".sc", "application/vnd.ibm.secure-container" },
    { 32260, 32260, ".icc", "application/vnd.iccprofile" },
    { 32261, 32260, ".icm", "application/vnd.iccprofile" },
    { 32270, 32270, ".igl", "application/vnd.igloader" },
    { 32280, 32280, ".ivp", "application/vnd.immervision-ivp" },
    { 32290, 32290, ".ivu", "application/vnd.immervision-ivu" },
    { 32300, 32300, ".igm", "application/vnd.insors.igm" },
    { 32310, 32310, ".xpw", "application/vnd.intercon.formnet" },
    { 32311, 32310, ".xpx", "application/vnd.intercon.formnet" },
    { 32320, 32320, ".i2g", "application/vnd.intergeo" },
    { 32330, 32330, ".qbo", "application/vnd.intu.qbo" },
    { 32340, 32340, ".qfx", "application/vnd.intu.qfx" },
    { 32350, 32350, ".rcprofile", "application/vnd.ipunplugged.rcprofile" },
    { 32360, 32360, ".irp", "application/vnd.irepository.package+xml" },
    { 32370, 32370, ".xpr", "application/vnd.is-xpr" },
    { 32380, 32380, ".fcs", "application/vnd.isac.fcs" },
    { 32390, 32390, ".jam", "application/vnd.jam" },
    { 32400, 32400, ".jisp", "application/vnd.jisp" },
    { 32410, 32410, ".joda", "application/vnd.joost.joda-archive" },
    { 32420, 32420, ".ktz", "application/vnd.kahootz" },
    { 32421, 32420, ".ktr", "application/vnd.kahootz" },
    { 32430, 32430, ".karbon", "application/vnd.kde.karbon" },
    { 32440, 32440, ".chrt", "application/vnd.kde.kchart" },
    { 32450, 32450, ".kfo", "application/vnd.kde.kformula" },
    { 32460, 32460, ".flw", "application/vnd.kde.kivio" },
    { 32470, 32470, ".kon", "application/vnd.kde.kontour" },
    { 32480, 32480, ".kpr", "application/vnd.kde.kpresenter" },
    { 32481, 32480, ".kpt", "application/vnd.kde.kpresenter" },
    { 32490, 32490, ".ksp", "application/vnd.kde.kspread" },
    { 32500, 32500, ".kwd", "application/vnd.kde.kword" },
    { 32501, 32500, ".kwt", "application/vnd.kde.kword" },
    { 32510, 32510, ".htke", "application/vnd.kenameaapp" },
    { 32520, 32520, ".kia", "application/vnd.kidspiration" },
    { 32530, 32530, ".kne", "application/vnd.kinar" },
    { 32531, 32530, ".knp", "application/vnd.kinar" },
    { 32540, 32540, ".skp", "application/vnd.koan" },
    { 32541, 32540, ".skd", "application/vnd.koan" },
    { 32542, 32540, ".skt", "application/vnd.koan" },
    { 32543, 32540, ".skm", "application/vnd.koan" },
    { 32550, 32550, ".sse", "application/vnd.kodak-descriptor" },
    { 32560, 32560, ".lasxml", "application/vnd.las.las+xml" },
    { 32570, 32570, ".lbd", "application/vnd.llamagraphics.life-balance.desktop" },
    { 32580, 32580, ".lbe", "application/vnd.llamagraphics.life-balance.exchange+xml" },

    { 32590, 32590, ".portpkg", "application/vnd.macports.portpkg" },
    { 32600, 32600, ".mcd", "application/vnd.mcd" },
    { 32610, 32610, ".mc1", "application/vnd.medcalcdata" },
    { 32620, 32620, ".cdkey", "application/vnd.mediastation.cdkey" },
    { 32630, 32630, ".mwf", "application/vnd.mfer" },
    { 32640, 32640, ".mfm", "application/vnd.mfmp" },
    { 32650, 32650, ".flo", "application/vnd.micrografx.flo" },
    { 32660, 32660, ".igx", "application/vnd.micrografx.igx" },
    { 32670, 32670, ".mif", "application/vnd.mif" },
    { 32680, 32680, ".daf", "application/vnd.mobius.daf" },
    { 32690, 32690, ".dis", "application/vnd.mobius.dis" },
    { 32700, 32700, ".mbk", "application/vnd.mobius.mbk" },
    { 32710, 32710, ".mqy", "application/vnd.mobius.mqy" },
    { 32720, 32720, ".msl", "application/vnd.mobius.msl" },
    { 32730, 32730, ".plc", "application/vnd.mobius.plc" },
    { 32740, 32740, ".txf", "application/vnd.mobius.txf" },
    { 32750, 32750, ".mpn", "application/vnd.mophun.application" },
    { 32760, 32760, ".mpc", "application/vnd.mophun.certificate" },
    { 32770, 32770, ".xul", "application/vnd.mozilla.xul+xml" },

    { 32780, 32780, ".mseq", "application/vnd.mseq" },
    { 32790, 32790, ".mus", "application/vnd.musician" },
    { 32800, 32800, ".msty", "application/vnd.muvee.style" },
    { 32810, 32810, ".taglet", "application/vnd.mynfc" },
    { 32820, 32820, ".nlu", "application/vnd.neurolanguage.nlu" },
    { 32830, 32830, ".ntf", "application/vnd.nitf" },
    { 32840, 32840, ".nitf", "application/vnd.nitf" },
    { 32850, 32850, ".nnd", "application/vnd.noblenet-directory" },
    { 32860, 32860, ".nns", "application/vnd.noblenet-sealer" },
    { 32870, 32870, ".nnw", "application/vnd.noblenet-web" },
    { 32880, 32880, ".ngdat", "application/vnd.nokia.n-gage.data" },
    { 32890, 32890, ".rpst", "application/vnd.nokia.radio-preset" },
    { 32900, 32900, ".rpss", "application/vnd.nokia.radio-presets" },
    { 32910, 32910, ".edm", "application/vnd.novadigm.edm" },
    { 32920, 32920, ".edx", "application/vnd.novadigm.edx" },
    { 32930, 32930, ".ext", "application/vnd.novadigm.ext" },

    { 32940, 32940, ".xo", "application/vnd.olpc-sugar" },
    { 32950, 32950, ".dd2", "application/vnd.oma.dd2+xml" },

    { 32960, 32960, ".mgp", "application/vnd.osgeo.mapguide.package" },
    { 32970, 32970, ".dp", "application/vnd.osgi.dp" },
    { 32980, 32980, ".esa", "application/vnd.osgi.subsystem" },
    { 32990, 32990, ".pdb", "application/vnd.palm" },
    { 32991, 32990, ".pqa", "application/vnd.palm" },
    { 32992, 32990, ".oprc", "application/vnd.palm" },
    { 33000, 33000, ".paw", "application/vnd.pawaafile" },
    { 33010, 33010, ".str", "application/vnd.pg.format" },
    { 33020, 33020, ".ei6", "application/vnd.pg.osasli" },
    { 33030, 33030, ".efif", "application/vnd.picsel" },
    { 33040, 33040, ".wg", "application/vnd.pmi.widget" },
    { 33050, 33050, ".plf", "application/vnd.pocketlearn" },
    { 33060, 33060, ".pbd", "application/vnd.powerbuilder6" },
    { 33070, 33070, ".box", "application/vnd.previewsystems.box" },
    { 33080, 33080, ".mgz", "application/vnd.proteus.magazine" },
    { 33090, 33090, ".qps", "application/vnd.publishare-delta-tree" },
    { 33100, 33100, ".ptid", "application/vnd.pvi.ptid1" },
    { 33120, 33120, ".qxd", "application/vnd.quark.quarkxpress" },
    { 33121, 33120, ".qxt", "application/vnd.quark.quarkxpress" },
    { 33122, 33120, ".qwd", "application/vnd.quark.quarkxpress" },
    { 33123, 33120, ".qwt", "application/vnd.quark.quarkxpress" },
    { 33124, 33120, ".qxl", "application/vnd.quark.quarkxpress" },
    { 33125, 33120, ".qxb", "application/vnd.quark.quarkxpress" },
    { 33130, 33130, ".bed", "application/vnd.realvnc.bed" },
    { 33140, 33140, ".mxl", "application/vnd.recordare.musicxml" },
    { 33150, 33150, ".musicxml", "application/vnd.recordare.musicxml+xml" },
    { 33160, 33160, ".cryptonote", "application/vnd.rig.cryptonote" },
    { 33170, 33170, ".cod", "application/vnd.rim.cod" },
    { 33200, 33200, ".link66", "application/vnd.route66.link66+xml" },
    { 33210, 33210, ".st", "application/vnd.sailingtracker.track" },
    { 33220, 33220, ".see", "application/vnd.seemail" },
    { 33230, 33230, ".sema", "application/vnd.sema" },
    { 33240, 33240, ".semd", "application/vnd.semd" },
    { 33250, 33250, ".semf", "application/vnd.semf" },
    { 33260, 33260, ".ifm", "application/vnd.shana.informed.formdata" },
    { 33270, 33270, ".itp", "application/vnd.shana.informed.formtemplate" },
    { 33280, 33280, ".iif", "application/vnd.shana.informed.interchange" },
    { 33290, 33290, ".ipk", "application/vnd.shana.informed.package" },
    { 33300, 33300, ".twd", "application/vnd.simtech-mindmapper" },
    { 33310, 33310, ".twds", "application/vnd.simtech-mindmapper" },
    { 33320, 33320, ".mmf", "application/vnd.smaf" },
    { 33330, 33330, ".teacher", "application/vnd.smart.teacher" },
    { 33340, 33340, ".sdkm", "application/vnd.solent.sdkm+xml" },
    { 33341, 33340, ".sdkd", "application/vnd.solent.sdkm+xml" },
    { 33350, 33350, ".dxp", "application/vnd.spotfire.dxp" },
    { 33360, 33360, ".sfs", "application/vnd.spotfire.sfs" },
    { 33370, 33370, ".sdc", "application/vnd.stardivision.calc" },
    { 33380, 33380, ".sda", "application/vnd.stardivision.draw" },
    { 33390, 33390, ".sdd", "application/vnd.stardivision.impress" },
    { 33410, 33410, ".smf", "application/vnd.stardivision.math" },
    { 33420, 33420, ".sdw", "application/vnd.stardivision.writer" },
    { 33421, 33420, ".vor", "application/vnd.stardivision.writer" },
    { 33430, 33430, ".sgl", "application/vnd.stardivision.writer-global" },
    { 33440, 33440, ".smzip", "application/vnd.stepmania.package" },
    { 33450, 33450, ".sm", "application/vnd.stepmania.stepchart" },
    { 33460, 33460, ".sxc", "application/vnd.sun.xml.calc" },
    { 33470, 33470, ".stc", "application/vnd.sun.xml.calc.template" },
    { 33480, 33480, ".sxd", "application/vnd.sun.xml.draw" },
    { 33490, 33490, ".std", "application/vnd.sun.xml.draw.template" },
    { 33500, 33500, ".sxi", "application/vnd.sun.xml.impress" },
    { 33560, 33560, ".sti", "application/vnd.sun.xml.impress.template" },
    { 33570, 33570, ".sxm", "application/vnd.sun.xml.math" },
    { 33580, 33580, ".sxw", "application/vnd.sun.xml.writer" },
    { 33590, 33590, ".sxg", "application/vnd.sun.xml.writer.global" },
    { 33600, 33600, ".stw", "application/vnd.sun.xml.writer.template" },
    { 33610, 33610, ".sus", "application/vnd.sus-calendar" },
    { 33620, 33620, ".susp", "application/vnd.sus-calendar" },
    { 33630, 33630, ".svd", "application/vnd.svd" },

    { 33640, 33640, ".tao", "application/vnd.tao.intent-module-archive" },
    { 33650, 33650, ".pcap", "application/vnd.tcpdump.pcap" },
    { 33651, 33650, ".cap", "application/vnd.tcpdump.pcap" },
    { 33652, 33650, ".dmp", "application/vnd.tcpdump.pcap" },
    { 33660, 33660, ".tmo", "application/vnd.tmobile-livetv" },
    { 33670, 33670, ".tpt", "application/vnd.trid.tpt" },
    { 33680, 33680, ".mxs", "application/vnd.triscape.mxs" },
    { 33690, 33690, ".tra", "application/vnd.trueapp" },
    { 33700, 33700, ".ufd", "application/vnd.ufdl" },
    { 33700, 33700, ".ufdl", "application/vnd.ufdl" },
    { 33710, 33710, ".utz", "application/vnd.uiq.theme" },
    { 33720, 33720, ".umj", "application/vnd.umajin" },
    { 33730, 33730, ".unityweb", "application/vnd.unity" },
    { 33740, 33740, ".uoml", "application/vnd.uoml+xml" },
    { 33750, 33750, ".vcx", "application/vnd.vcx" },

    { 33760, 33760, ".vis", "application/vnd.visionary" },
    { 33770, 33770, ".vsf", "application/vnd.vsf" },

    { 33780, 33780, ".wtb", "application/vnd.webturbo" },
    { 33790, 33790, ".nbp", "application/vnd.wolfram.player" },
    { 33800, 33800, ".wpd", "application/vnd.wordperfect" },
    { 33810, 33810, ".wqd", "application/vnd.wqd" },
    { 33820, 33820, ".stf", "application/vnd.wt.stf" },
    { 33830, 33830, ".xar", "application/vnd.xara" },
    { 33840, 33840, ".xfdl", "application/vnd.xfdl" },
    { 33850, 33850, ".hvd", "application/vnd.yamaha.hv-dic" },
    { 33860, 33860, ".hvs", "application/vnd.yamaha.hv-script" },
    { 33870, 33870, ".hvp", "application/vnd.yamaha.hv-voice" },
    { 33880, 33880, ".osf", "application/vnd.yamaha.openscoreformat" },
    { 33890, 33890, ".osfpvg", "application/vnd.yamaha.openscoreformat.osfpvg+xml" },
    { 33900, 33900, ".saf", "application/vnd.yamaha.smaf-audio" },
    { 33910, 33910, ".spf", "application/vnd.yamaha.smaf-phrase" },
    { 33920, 33920, ".cmp", "application/vnd.yellowriver-custom-menu" },
    { 33930, 33930, ".zir", "application/vnd.zul" },
    { 33940, 33940, ".zirz", "application/vnd.zul" },
    { 33950, 33950, ".zaz", "application/vnd.zzazz.deck+xml" },
    { 33960, 33960, ".wgt", "application/widget" },

    { 33970, 33970, ".abw", "application/x-abiword" },

    { 33980, 33980, ".dmg", "application/x-apple-diskimage" },
    { 33990, 33990, ".aab", "application/x-authorware-bin" },
    { 33991, 33990, ".x32", "application/x-authorware-bin" },
    { 33992, 33990, ".u32", "application/x-authorware-bin" },
    { 33993, 33990, ".vox", "application/x-authorware-bin" },
    { 34000, 34000, ".aam", "application/x-authorware-map" },
    { 34010, 34010, ".aas", "application/x-authorware-seg" },
    { 34020, 34020, ".bcpio", "application/x-bcpio" },
    { 34030, 34030, ".torrent", "application/x-bittorrent" },
    { 34040, 34040, ".blb", "application/x-blorb" },
    { 34041, 34040, ".blorb", "application/x-blorb" },


    { 34050, 34050, ".cbr", "application/x-cbr" },
    { 34051, 34050, ".cba", "application/x-cbr" },
    { 34052, 34050, ".cbt", "application/x-cbr" },
    { 34053, 34050, ".cbz", "application/x-cbr" },
    { 34054, 34050, ".cb7", "application/x-cbr" },
    { 34060, 34060, ".vcd", "application/x-cdlink" },
    { 34070, 34070, ".chat", "application/x-chat" },
    { 34080, 34080, ".pgn", "application/x-chess-pgn" },
    { 34090, 34090, ".nsc", "application/x-conference" },
    { 34100, 34100, ".cpio", "application/x-cpio" },
    { 34110, 34110, ".csh", "application/x-csh" },
    { 34120, 34120, ".deb", "application/x-debian-package" },
    { 34130, 34130, ".udeb", "application/x-debian-package" },
    { 34140, 34140, ".dir", "application/x-director" },
    { 34141, 34140, ".dcr", "application/x-director" },
    { 34142, 34140, ".dxr", "application/x-director" },
    { 34143, 34140, ".cst", "application/x-director" },
    { 34144, 34140, ".cct", "application/x-director" },
    { 34145, 34140, ".cxt", "application/x-director" },
    { 34146, 34140, ".w3d", "application/x-director" },
    { 34147, 34140, ".fgd", "application/x-director" },
    { 34148, 34140, ".swa", "application/x-director" },
    { 34150, 34150, ".wad", "application/x-doom" },
    { 34160, 34160, ".ncx", "application/x-dtbncx+xml" },
    { 34161, 34160, ".dtb", "application/x-dtbook+xml" },
    { 34170, 34170, ".res", "application/x-dtbresource+xml" },
    { 34180, 34180, ".dvi", "application/x-dvi" },
    { 34190, 34190, ".evy", "application/x-envoy" },
    { 34200, 34200, ".eva", "application/x-eva" },

    { 34210, 34210, ".bdf", "application/x-font-bdf" },
    { 34220, 34220, ".gsf", "application/x-font-ghostscript" },
    { 34230, 34230, ".psf", "application/x-font-linux-psf" },
    { 34240, 34240, ".otf", "application/x-font-otf" },
    { 34250, 34250, ".pcf", "application/x-font-pcf" },
    { 34260, 34260, ".snf", "application/x-font-snf" },
    { 34270, 34270, ".ttf", "application/x-font-ttf" },
    { 34271, 34270, ".ttc", "application/x-font-ttf" },
    { 34280, 34280, ".pfa", "application/x-font-type1" },
    { 34281, 34280, ".pfb", "application/x-font-type1" },
    { 34282, 34280, ".pfm", "application/x-font-type1" },
    { 34283, 34280, ".afm", "application/x-font-type1" },
    { 34290, 34290, ".woff", "application/x-font-woff" },

    { 34300, 34300, ".arc", "application/x-freearc" },
    { 34310, 34310, ".spl", "application/x-futuresplash" },
    { 34320, 34320, ".ulx", "application/x-glulx" },
    { 34330, 34330, ".gnumeric", "application/x-gnumeric" },
    { 34340, 34340, ".gramps", "application/x-gramps-xml" },
    { 34360, 34360, ".hdf", "application/x-hdf" },
    { 34370, 34370, ".iso", "application/x-iso9660-image" },
    { 34380, 34380, ".latex", "application/x-latex" },
    { 34381, 34381, ".ltx", "application/x-latex" },

    { 34390, 34390, ".mie", "application/x-mie" },
    { 34400, 34400, ".prc", "application/x-mobipocket-ebook" },
    { 34401, 34400, ".mobi", "application/x-mobipocket-ebook" },

    { 34410, 34410, ".nc", "application/x-netcdf" },
    { 34411, 34410, ".cdf", "application/x-netcdf" },
    { 34420, 34420, ".nzb", "application/x-nzb" },
    { 34430, 34430, ".p12", "application/x-pkcs12" },
    { 34431, 34430, ".pfx", "application/x-pkcs12" },
    { 34440, 34440, ".p7b", "application/x-pkcs7-certificates" },
    { 34441, 34440, ".spc", "application/x-pkcs7-certificates" },
    { 34450, 34450, ".p7r", "application/x-pkcs7-certreqresp" },
    { 34460, 34460, ".ris", "application/x-research-info-systems" },
    { 34470, 34470, ".sh", "application/x-sh" },
    { 34480, 34480, ".shar", "application/x-shar" },
    { 34490, 34490, ".xap", "application/x-silverlight-app" },
    { 34500, 34500, ".sql", "application/x-sql" },
    { 34510, 34510, ".sit", "application/x-stuffit" },
    { 34520, 34520, ".sitx", "application/x-stuffitx" },
    { 34530, 34530, ".srt", "application/x-subrip" },
    { 34540, 34540, ".sv4cpio", "application/x-sv4cpio" },
    { 34550, 34550, ".sv4crc", "application/x-sv4crc" },
    { 34560, 34560, ".t3", "application/x-t3vm-image" },
    { 34570, 34570, ".gam", "application/x-tads" },
    { 34580, 34580, ".tcl", "application/x-tcl" },
    { 34590, 34590, ".tex", "application/x-tex" },
    { 34600, 34600, ".tfm", "application/x-tex-tfm" },
    { 34610, 34610, ".texinfo", "application/x-texinfo" },
    { 34620, 34620, ".texi", "application/x-texinfo" },
    { 34630, 34630, ".obj", "application/x-tgif" },
    { 34640, 34640, ".src", "application/x-wais-source" },
    { 34650, 34650, ".der", "application/x-x509-ca-cert" },
    { 34651, 34650, ".crt", "application/x-x509-ca-cert" },
    { 34660, 34660, ".fig", "application/x-xfig" },
    { 34670, 34670, ".xlf", "application/x-xliff+xml" },
    { 34680, 34680, ".z1", "application/x-zmachine" },
    { 34681, 34680, ".z2", "application/x-zmachine" },
    { 34682, 34680, ".z3", "application/x-zmachine" },
    { 34683, 34680, ".z4", "application/x-zmachine" },
    { 34684, 34680, ".z5", "application/x-zmachine" },
    { 34685, 34680, ".z6", "application/x-zmachine" },
    { 34686, 34680, ".z7", "application/x-zmachine" },
    { 34687, 34680, ".z8", "application/x-zmachine" },
    { 34690, 34690, ".xaml", "application/xaml+xml" },

    { 34700, 34700, ".yang", "application/yang" },

    { 34710, 34710, ".cdx", "chemical/x-cdx" },
    { 34720, 34720, ".cif", "chemical/x-cif" },
    { 34730, 34730, ".cmdf", "chemical/x-cmdf" },
    { 34740, 34740, ".cml", "chemical/x-cml" },
    { 34750, 34750, ".csml", "chemical/x-csml" },
    { 34760, 34760, ".xyz", "chemical/x-xyz" },


    { 34770, 34770, ".eml", "message/rfc822" },
    { 34771, 34770, ".mime", "message/rfc822" },

    { 34790, 34790, ".igs", "model/iges" },
    { 34791, 34790, ".iges", "model/iges" },
    { 34800, 34800, ".msh", "model/mesh" },
    { 34801, 34800, ".mesh", "model/mesh" },
    { 34802, 34800, ".silo", "model/mesh" },
    { 34810, 34810, ".dae", "model/vnd.collada+xml" },
    { 34820, 34820, ".dwf", "model/vnd.dwf" },
    { 34830, 34830, ".gdl", "model/vnd.gdl" },
    { 34840, 34840, ".gtw", "model/vnd.gtw" },
    { 34850, 34850, ".mts", "model/vnd.mts" },
    { 34860, 34860, ".vtu", "model/vnd.vtu" },
    { 34870, 34870, ".wrl", "model/vrml" },
    { 34871, 34870, ".vrml", "model/vrml" },
    { 34880, 34880, ".x3db", "model/x3d+binary" },
    { 34881, 34880, ".x3dbz", "model/x3d+binary" },
    { 34890, 34890, ".x3dv", "model/x3d+vrml" },
    { 34891, 34890, ".x3dvz", "model/x3d+vrml" },
    { 34900, 34900, ".x3d", "model/x3d+xml" },
    { 34901, 34900, ".x3dz", "model/x3d+xml" },

    { 41000, 41000, ".xdma", "application/x-xdma" },
    { 41010, 41010, ".pl", "application/x-perl" },
    { 41020, 41020, ".vdb", "application/activexdocument" },
};
#define MIMENUM (sizeof(g_mime)/sizeof(g_mime[0]))


static int mime_item_cmp_extname (void * a, void * b)
{
    MimeItem * item = (MimeItem *)a;
    char    * ext = (char *)b;

    if (!item) return -1;
    if (!ext) return 1;

    return strcasecmp(item->extname, ext);
}

static int mime_item_cmp_mimeid (void * a, void * b)
{
    MimeItem * item = (MimeItem *)a;
    uint32     mimeid  = *(uint32 *)b;

    if (!item) return -1;

    if (item->mimeid > mimeid) return 1;
    if (item->mimeid < mimeid) return -1;
    return 0;
}

static int mime_item_cmp_mimetype (void * a, void * b)
{
    MimeItem * item = (MimeItem *)a;
    char    * mtype = (char *)b;

    if (!item) return -1;
    if (!mtype) return 1;

    return strcasecmp(item->mime, mtype);
}

static ulong mimeid_hash_func (void * key)
{
    uint32 mimeid = *(uint32 *)key;
    return (ulong)mimeid;
}


void * mime_type_init ()
{
    MimeMgmt * mgmt = NULL;
    int        i = 0;
    MimeItem * item = NULL;

    if (g_mimemgmt_init && g_mimemgmt)
        return g_mimemgmt;

    mgmt = kzalloc(sizeof(*mgmt));
    if (!mgmt) return NULL;

    g_mimemgmt = mgmt;
    g_mimemgmt_init = 1;

    mgmt->mime_tab = ht_only_new(1200, mime_item_cmp_extname);

    mgmt->mimeid_tab = ht_only_new(1100, mime_item_cmp_mimeid);
    ht_set_hash_func(mgmt->mimeid_tab, mimeid_hash_func);

    mgmt->mimetype_tab = ht_only_new(1200, mime_item_cmp_mimetype);

    for (i = 0; i < MIMENUM; i++) {
        item = &g_mime[i];

        if (ht_get(mgmt->mime_tab, item->extname) == NULL)
            ht_set(mgmt->mime_tab, item->extname, item);

        ht_set(mgmt->mimeid_tab, &item->mimeid, item);

        if (ht_get(mgmt->mimetype_tab, item->mime) == NULL)
            ht_set(mgmt->mimetype_tab, item->mime, item);
    }

    return mgmt;
}


int mime_type_clean (void * vmgmt)
{
    MimeMgmt * mgmt = (MimeMgmt *)vmgmt;

    if (!mgmt) return -1;

    if (g_mimemgmt == mgmt) {
        g_mimemgmt = NULL;
        g_mimemgmt_init = 0;
    }

    ht_free(mgmt->mime_tab);
    ht_free(mgmt->mimeid_tab);
    ht_free(mgmt->mimetype_tab);

    kfree(mgmt);
    return 0;
}
 
void * mime_type_alloc (int tabsize)
{
    MimeMgmt * mgmt = NULL;
 
    mgmt = kzalloc(sizeof(*mgmt));
    if (!mgmt) return NULL;
 
    if (tabsize < 200) tabsize = 200;

    mgmt->mime_tab = ht_only_new(tabsize, mime_item_cmp_extname);
 
    mgmt->mimeid_tab = ht_only_new(tabsize, mime_item_cmp_mimeid);
    ht_set_hash_func(mgmt->mimeid_tab, mimeid_hash_func);
 
    mgmt->mimetype_tab = ht_only_new(tabsize, mime_item_cmp_mimetype);

    mgmt->mime_list = arr_new(4);

    return mgmt;
}

void mime_type_free (void * vmgmt)
{
    MimeMgmt * mgmt = (MimeMgmt *)vmgmt;
    MimeItem * item = NULL;
    int        i, num;

    if (!mgmt) return;

    num = arr_num(mgmt->mime_list);
 
    for (i = 0; i < num; i++) {
        item = arr_value(mgmt->mime_list, i);
        if (!item) continue;
 
        kfree(item);
    }
    arr_free(mgmt->mime_list);

    ht_free(mgmt->mimetype_tab);
    ht_free(mgmt->mime_tab);
    ht_free(mgmt->mimeid_tab);

    kfree(mgmt);
}

int mime_type_add (void * vmgmt, char * mime, char * ext, uint32 mimeid, uint32 appid)
{
    MimeMgmt * mgmt = (MimeMgmt *)vmgmt;
    MimeItem * item = NULL;
    uint8      setflag = 0;

    if (!mgmt) return -1;

    if (!mime || strlen(mime) <= 0) return -2;
    if (!ext || strlen(ext) <= 0) return -3;

    item = kzalloc(sizeof(*item));
    if (!item) return -100;

    str_secpy(item->mime, sizeof(item->mime)-1, mime, strlen(mime));
    str_secpy(item->extname, sizeof(item->extname)-1, ext, strlen(ext));
    item->mimeid = mimeid;
    item->appid = appid;
    
    if (ht_get(mgmt->mimetype_tab, mime) == NULL) {
        ht_set(mgmt->mimetype_tab, item->mime, item);
        setflag |= 0x01;
    }

    if (ht_get(mgmt->mime_tab, ext) == NULL) {
        ht_set(mgmt->mime_tab, item->extname, item);
        setflag |= 0x02;
    }

    if (mimeid > 0 && ht_get(mgmt->mimeid_tab, &mimeid) == NULL) {
        ht_set(mgmt->mimeid_tab, &mimeid, item);
        setflag |= 0x04;
    }

    if (setflag == 0) {
        kfree(item);
    } else {
        arr_push(mgmt->mime_list, item);
    }

    return 0;
}

int mime_type_get_by_extname (void * vmgmt, char * ext, char ** pmime, uint32 * mimeid, uint32 * appid)
{
    MimeMgmt * mgmt = (MimeMgmt *)vmgmt;
    static char * default_mime = "application/octet-stream";
    MimeItem * item = NULL;
    char    * p = NULL;

    if (pmime) *pmime = default_mime;
    if (mimeid) *mimeid = 0;
    if (appid) *appid = 0;

    if (!mgmt) return -1;
    if (!ext) return -2;

    p = ext;
    if (*p != '.') p = rskipTo(p+strlen(ext)-1, strlen(ext), ".", 1);
    if (*p == '.') item = ht_get(mgmt->mime_tab, p);
    if (!item) return -100;
    
    if (pmime) *pmime = item->mime;
    if (mimeid) *mimeid = item->mimeid;
    if (appid) *appid = item->appid;

    return 0;
}

int mime_type_get_by_mimeid (void * vmgmt, uint32 mimeid, char ** pmime, char ** pext, uint32 * appid)
{
    MimeMgmt * mgmt = (MimeMgmt *)vmgmt;
    MimeItem * item = NULL;
    static char * default_mime = "application/octet-stream";
    static char * default_extname = ".bin";

    if (pmime) *pmime = default_mime;
    if (pext) *pext = default_extname;
    if (appid) *appid = 0;

    if (!mgmt) return -1;

    item = ht_get(mgmt->mimeid_tab, &mimeid);
    if (!item) return -100;

    if (pmime) *pmime = item->mime;
    if (pext) *pext = item->extname;
    if (appid) *appid = item->appid;
    
    return 0;
}


int mime_type_get_by_mime (void * vmgmt, char * mime, char ** pext, uint32 * mimeid, uint32 * appid)
{
    MimeMgmt * mgmt = (MimeMgmt *)vmgmt;
    MimeItem * item = NULL;
    static char * default_extname = ".bin";
    char       mimebuf[128];
    char     * p = NULL;
    int        len = 0;

    if (pext) *pext = default_extname;
    if (mimeid) *mimeid = 0;
    if (appid) *appid = 0;
    
    if (!mgmt) return -1;
    if (!mime) return -2;

    memset(mimebuf, 0, sizeof(mimebuf));

    mime = skipOver(mime, strlen(mime), " \t\r\n", 4);
    p = skipTo(mime, strlen(mime), ";, \t\r\n", 6);
    if (p) {
        len = p - mime;
        if (len > sizeof(mimebuf)-1) len = sizeof(mimebuf)-1;
        memcpy(mimebuf, mime, len);
    } else strncpy(mimebuf, mime, sizeof(mimebuf)-1);

    item = ht_get(mgmt->mimetype_tab, mimebuf);
    if (!item) return -100;

    if (pext) *pext = item->extname;
    if (mimeid) *mimeid = item->mimeid;
    if (appid) *appid = item->appid;

    return 0;
}


void * mime_type_get (void * vmgmt, char * mime, uint32 mimeid, char * ext)
{
    MimeMgmt * mgmt = (MimeMgmt *)vmgmt;
    MimeItem * item = NULL;

    if (!mgmt) return NULL;

    if (!item && mime) item = ht_get(mgmt->mimetype_tab, mime);
    if (!item && mimeid > 0) item = ht_get(mgmt->mimeid_tab, &mimeid);
    if (!item && ext) item = ht_get(mgmt->mime_tab, ext);

    return item;
}

