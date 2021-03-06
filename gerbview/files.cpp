/**
 * @file gerbview/files.cpp
 */

/*
 * This program source code file is part of KiCad, a free EDA CAD application.
 *
 * Copyright (C) 2017 Jean-Pierre Charras, jp.charras at wanadoo.fr
 * Copyright (C) 2004-2017 KiCad Developers, see AUTHORS.txt for contributors.
 *
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
 * along with this program; if not, you may find one here:
 * http://www.gnu.org/licenses/old-licenses/gpl-2.0.html
 * or you may search the http://www.gnu.org website for the version 2 license,
 * or you may write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 */

#include <fctsys.h>
#include <wx/fs_zip.h>
#include <wx/wfstream.h>
#include <wx/zipstrm.h>

#include <common.h>
#include <class_drawpanel.h>
#include <reporter.h>
#include <html_messagebox.h>

#include <gerbview_frame.h>
#include <gerbview_id.h>
#include <class_gerbview_layer_widget.h>
#include <wildcards_and_files_ext.h>

// HTML Messages used more than one time:
#define MSG_NO_MORE_LAYER\
    _( "<b>No more available free graphic layer</b> in Gerbview to load files" )
#define MSG_NOT_LOADED _( "\n<b>Not loaded:</b> <i>%s</i>" )

void GERBVIEW_FRAME::OnGbrFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Gerber files" ) );

    if( !fn.IsEmpty() )
    {
        Erase_Current_DrawLayer( false );
        LoadGerberFiles( fn );
    }
}


void GERBVIEW_FRAME::OnDrlFileHistory( wxCommandEvent& event )
{
    wxString fn;

    fn = GetFileFromHistory( event.GetId(), _( "Drill files" ), &m_drillFileHistory );

    if( !fn.IsEmpty() )
    {
        Erase_Current_DrawLayer( false );
        LoadExcellonFiles( fn );
    }
}

void GERBVIEW_FRAME::OnZipFileHistory( wxCommandEvent& event )
{
    wxString filename;
    filename = GetFileFromHistory( event.GetId(), _( "Zip files" ), &m_zipFileHistory );

    if( !filename.IsEmpty() )
    {
        Erase_Current_DrawLayer( false );
        LoadZipArchiveFile( filename );
    }
}


/* File commands. */
void GERBVIEW_FRAME::Files_io( wxCommandEvent& event )
{
    int        id = event.GetId();

    switch( id )
    {
    case wxID_FILE:
        Erase_Current_DrawLayer( false );
        LoadGerberFiles( wxEmptyString );
        break;

    case ID_GERBVIEW_ERASE_ALL:
        Clear_DrawLayers( false );
        Zoom_Automatique( false );
        m_canvas->Refresh();
        ClearMsgPanel();
        break;

    case ID_GERBVIEW_LOAD_DRILL_FILE:
        LoadExcellonFiles( wxEmptyString );
        m_canvas->Refresh();
        break;

    case ID_GERBVIEW_LOAD_ZIP_ARCHIVE_FILE:
        LoadZipArchiveFile( wxEmptyString );
        m_canvas->Refresh();
        break;

    default:
        wxFAIL_MSG( wxT( "File_io: unexpected command id" ) );
        break;
    }
}


bool GERBVIEW_FRAME::LoadGerberFiles( const wxString& aFullFileName )
{
    wxString   filetypes;
    wxArrayString filenamesList;
    wxFileName filename = aFullFileName;
    wxString currentPath;

    if( !filename.IsOk() )
    {
        /* Standard gerber filetypes
         * (See http://en.wikipedia.org/wiki/Gerber_File)
         * the .gbr (.pho in legacy files) extension is the default used in Pcbnew
         * However there are a lot of other extensions used for gerber files
         * Because the first letter is usually g, we accept g* as extension
         * (Mainly internal copper layers do not have specific extension,
         *  and filenames are like *.g1, *.g2 *.gb1 ...).
         * Now (2014) Ucamco (the company which manages the Gerber format) encourages
         * use of .gbr only and the Gerber X2 file format.
         */
        filetypes = _( "Gerber files (.g* .lgr .pho)" );
        filetypes << wxT("|");
        filetypes += wxT("*.g*;*.G*;*.pho;*.PHO" );
        filetypes << wxT("|");

        /* Special gerber filetypes */
        filetypes += _( "Top layer (*.GTL)|*.GTL;*.gtl|" );
        filetypes += _( "Bottom layer (*.GBL)|*.GBL;*.gbl|" );
        filetypes += _( "Bottom solder resist (*.GBS)|*.GBS;*.gbs|" );
        filetypes += _( "Top solder resist (*.GTS)|*.GTS;*.gts|" );
        filetypes += _( "Bottom overlay (*.GBO)|*.GBO;*.gbo|" );
        filetypes += _( "Top overlay (*.GTO)|*.GTO;*.gto|" );
        filetypes += _( "Bottom paste (*.GBP)|*.GBP;*.gbp|" );
        filetypes += _( "Top paste (*.GTP)|*.GTP;*.gtp|" );
        filetypes += _( "Keep-out layer (*.GKO)|*.GKO;*.gko|" );
        filetypes += _( "Mechanical layers (*.GMx)|*.GM1;*.gm1;*.GM2;*.gm2;*.GM3;*.gm3|" );
        filetypes += _( "Top Pad Master (*.GPT)|*.GPT;*.gpt|" );
        filetypes += _( "Bottom Pad Master (*.GPB)|*.GPB;*.gpb|" );

        // All filetypes
        filetypes += AllFilesWildcard;

        // Use the current working directory if the file name path does not exist.
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
        {
            currentPath = m_mruPath;

            // On wxWidgets 3.1 (bug?) the path in wxFileDialog is ignored when
            // finishing by the dir separator. Remove it if any:
            if( currentPath.EndsWith( '\\' ) || currentPath.EndsWith( '/' ) )
                currentPath.RemoveLast();
        }

        wxFileDialog dlg( this, _( "Open Gerber File" ),
                          currentPath,
                          filename.GetFullName(),
                          filetypes,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        dlg.GetPaths( filenamesList );

        // @todo Take a closer look at the CWD switching here.  The current working directory
        // gets changed by wxFileDialog because the wxFD_CHANGE_DIR flag is set.  Is this the
        // appropriate behavior?  The current working directory is not returned to the previous
        // value so this may be an issue elsewhere.
        currentPath = wxGetCwd();
        m_mruPath = currentPath;
    }
    else
    {
        filenamesList.Add( aFullFileName );
        currentPath = filename.GetPath();
        m_mruPath = currentPath;
    }

    // Read gerber files: each file is loaded on a new GerbView layer
    bool success = true;
    int layer = getActiveLayer();

    // Manage errors when loading files
    wxString msg;
    WX_STRING_REPORTER reporter( &msg );

    for( unsigned ii = 0; ii < filenamesList.GetCount(); ii++ )
    {
        filename = filenamesList[ii];

        if( !filename.IsAbsolute() )
            filename.SetPath( currentPath );

        m_lastFileName = filename.GetFullPath();

        setActiveLayer( layer, false );

        if( Read_GERBER_File( filename.GetFullPath() ) )
        {
            UpdateFileHistory( m_lastFileName );

            layer = getNextAvailableLayer( layer );

            if( layer == NO_AVAILABLE_LAYERS && ii < filenamesList.GetCount()-1 )
            {
                success = false;
                reporter.Report( MSG_NO_MORE_LAYER, REPORTER::RPT_ERROR );

                // Report the name of not loaded files:
                ii += 1;
                while( ii < filenamesList.GetCount() )
                {
                    filename = filenamesList[ii++];
                    wxString txt;
                    txt.Printf( MSG_NOT_LOADED,
                                GetChars( filename.GetFullName() ) );
                    reporter.Report( txt, REPORTER::RPT_ERROR );
                }
                break;
            }

            setActiveLayer( layer, false );
        }
    }

    if( !success )
    {
        HTML_MESSAGE_BOX mbox( this, _( "Errors" ) );
        mbox.ListSet( msg );
        mbox.ShowModal();
    }

    Zoom_Automatique( false );

    // Synchronize layers tools with actual active layer:
    ReFillLayerWidget();
    setActiveLayer( getActiveLayer() );
    m_LayersManager->UpdateLayerIcons();
    syncLayerBox();
    return success;
}


bool GERBVIEW_FRAME::LoadExcellonFiles( const wxString& aFullFileName )
{
    wxString   filetypes;
    wxArrayString filenamesList;
    wxFileName filename = aFullFileName;
    wxString currentPath;

    if( !filename.IsOk() )
    {
        filetypes = wxGetTranslation( DrillFileWildcard );
        filetypes << wxT("|");
        /* All filetypes */
        filetypes += wxGetTranslation( AllFilesWildcard );

        /* Use the current working directory if the file name path does not exist. */
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
            currentPath = m_mruPath;

        wxFileDialog dlg( this, _( "Open Drill File" ),
                          currentPath, filename.GetFullName(), filetypes,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_MULTIPLE | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        dlg.GetPaths( filenamesList );
        currentPath = wxGetCwd();
        m_mruPath = currentPath;
    }
    else
    {
        filenamesList.Add( aFullFileName );
        currentPath = filename.GetPath();
        m_mruPath = currentPath;
    }

    // Read Excellon drill files: each file is loaded on a new GerbView layer
    bool success = true;
    int layer = getActiveLayer();

    // Manage errors when loading files
    wxString msg;
    WX_STRING_REPORTER reporter( &msg );

    for( unsigned ii = 0; ii < filenamesList.GetCount(); ii++ )
    {
        filename = filenamesList[ii];

        if( !filename.IsAbsolute() )
            filename.SetPath( currentPath );

        m_lastFileName = filename.GetFullPath();

        setActiveLayer( layer, false );

        if( Read_EXCELLON_File( filename.GetFullPath() ) )
        {
            // Update the list of recent drill files.
            UpdateFileHistory( filename.GetFullPath(),  &m_drillFileHistory );

            layer = getNextAvailableLayer( layer );

            if( layer == NO_AVAILABLE_LAYERS && ii < filenamesList.GetCount()-1 )
            {
                success = false;
                reporter.Report( MSG_NO_MORE_LAYER, REPORTER::RPT_ERROR );

                // Report the name of not loaded files:
                ii += 1;
                while( ii < filenamesList.GetCount() )
                {
                    filename = filenamesList[ii++];
                    wxString txt;
                    txt.Printf( MSG_NOT_LOADED,
                                GetChars( filename.GetFullName() ) );
                    reporter.Report( txt, REPORTER::RPT_ERROR );
                }
                break;
            }

            setActiveLayer( layer, false );
        }
    }

    if( !success )
    {
        HTML_MESSAGE_BOX mbox( this, _( "Errors" ) );
        mbox.ListSet( msg );
        mbox.ShowModal();
    }

    Zoom_Automatique( false );

    // Synchronize layers tools with actual active layer:
    ReFillLayerWidget();
    setActiveLayer( getActiveLayer() );
    m_LayersManager->UpdateLayerIcons();
    syncLayerBox();

    return success;
}


bool GERBVIEW_FRAME::unarchiveFiles( const wxString& aFullFileName, REPORTER* aReporter )
{
    wxFileSystem zipfilesys;

    zipfilesys.AddHandler( new wxZipFSHandler );
    zipfilesys.ChangePathTo( aFullFileName + wxT( "#zip:" ), true );

    wxString  localfilename = zipfilesys.FindFirst( wxT( "*.*" ) );
    wxFileName fn( aFullFileName );
    wxString unzipDir = fn.GetPath();

    // Update the list of recent zip files.
    UpdateFileHistory( aFullFileName,  &m_zipFileHistory );

    bool success = true;
    wxString msg;

    while( !localfilename.IsEmpty() )
    {
        wxFSFile* zipfile = zipfilesys.OpenFile( localfilename );

        if( !zipfile )
        {
            if( aReporter )
            {
                msg.Printf( _( "Zip file '%s' cannot be read" ), GetChars( aFullFileName ) );
                aReporter->Report( msg, REPORTER::RPT_ERROR );
            }
            success = false;
            break;
        }

        // In order to load a file in this archive, this file is unzipped and
        // a temporary file is created in the same folder as the archive.
        // This file will be deleted after being loaded in the viewer.
        // One other (and better) way is to load it from the memory image, but currently
        // Read_GERBER_File and Read_EXCELLON_File expects a file.
        wxFileName uzfn = localfilename.AfterLast( ':' );
        uzfn.MakeAbsolute( unzipDir );

        // The unzipped file in only a temporary file. Give it a filename
        // which cannot conflict with an usual gerber or drill file
        // by adding a '$' to its ext.
        wxString unzipfilename = uzfn.GetFullPath() + "$";

        wxInputStream* stream = zipfile->GetStream();
        wxFFileOutputStream* temporary_ofile = new wxFFileOutputStream( unzipfilename );

        if( temporary_ofile->Ok() )
            temporary_ofile->Write( *stream );
        else
        {
            success = false;
        }

        // Close streams:
        delete temporary_ofile;
        delete zipfile;

        // The archive contains Gerber and/or Excellon drill files. Use the right loader.
        // However it can contain a few other files (reports, pdf files...),
        // which will be skipped.
        // Gerber files ext is usually "gbr", but can be also an other value, starting by "g"
        // old gerber files ext from kicad is .pho
        // drill files do not have a well defined ext
        // It is .drl in kicad, but .txt in Altium for instance
        // Allows only .drl for drill files.
        int layer = getActiveLayer();
        setActiveLayer( layer, false );
        bool read_ok = true;
        bool skip_file = false;

        wxString curr_ext = uzfn.GetExt().Lower();

        if( curr_ext[0] == 'g' || curr_ext == "pho" )
        {
            // Read gerber files: each file is loaded on a new GerbView layer
            read_ok = Read_GERBER_File( unzipfilename );
        }
        else if( curr_ext == "drl" )
        {
            read_ok = Read_EXCELLON_File( unzipfilename );
        }
        else    // if the ext is not "pho", "g*" or "drl",
        {
                // the type is unknown for Gerbview. Skip it
            skip_file = true;
            success = false;

            if( aReporter )
            {
                msg.Printf( _( "Info: skip <i>%s</i> (unknown type)\n" ),
                            GetChars( localfilename.AfterLast( ':' ) ) );
                aReporter->Report( msg, REPORTER::RPT_WARNING );
            }
        }

        // The unzipped file is only a temporary file, delete it.
        wxRemoveFile( unzipfilename );

        if( !read_ok && !skip_file)
        {
            success = false;

            if( aReporter )
            {
                msg.Printf( _("<b>file %s read error</b>\n"), GetChars( unzipfilename ) );
                aReporter->Report( msg, REPORTER::RPT_ERROR );
            }
        }

        // Prepare the loading of the next file in archive, if exists
        localfilename = zipfilesys.FindNext();

        if( !skip_file )
        {
            if( read_ok )
            {
                layer = getNextAvailableLayer( layer );

                if( layer == NO_AVAILABLE_LAYERS && !localfilename.IsEmpty() )
                {
                    success = false;

                    if( aReporter )
                    {
                        aReporter->Report( MSG_NO_MORE_LAYER, REPORTER::RPT_ERROR );

                        // Report the name of not loaded files:
                        while( !localfilename.IsEmpty() )
                        {
                            msg.Printf( MSG_NOT_LOADED,
                                        GetChars( localfilename.AfterLast( ':' ) ) );
                            aReporter->Report( msg, REPORTER::RPT_ERROR );
                            localfilename = zipfilesys.FindNext();
                        }
                    }
                    break;
                }
            }

            setActiveLayer( layer, false );
        }
    }

    return success;
}


bool GERBVIEW_FRAME::LoadZipArchiveFile( const wxString& aFullFileName )
{
#define ZipFileExtension "zip"
#define ZipFileWildcard  _( "Zip file (*.zip)|*.zip" )
    wxFileName filename = aFullFileName;
    wxString currentPath;

    if( !filename.IsOk() )
    {
        // Use the current working directory if the file name path does not exist.
        if( filename.DirExists() )
            currentPath = filename.GetPath();
        else
            currentPath = m_mruPath;

        wxFileDialog dlg( this,
                          _( "Open Zip File" ),
                          currentPath,
                          filename.GetFullName(),
                          ZipFileWildcard,
                          wxFD_OPEN | wxFD_FILE_MUST_EXIST | wxFD_CHANGE_DIR );

        if( dlg.ShowModal() == wxID_CANCEL )
            return false;

        filename = dlg.GetPath();
        currentPath = wxGetCwd();
        m_mruPath = currentPath;
    }
    else
    {
        currentPath = filename.GetPath();
        m_mruPath = currentPath;
    }

    wxString msg;
    WX_STRING_REPORTER reporter( &msg );

    if( filename.IsOk() )
        unarchiveFiles( filename.GetFullPath(), &reporter );

    Zoom_Automatique( false );

    // Synchronize layers tools with actual active layer:
    ReFillLayerWidget();
    setActiveLayer( getActiveLayer() );
    m_LayersManager->UpdateLayerIcons();
    syncLayerBox();

    if( !msg.IsEmpty() )
    {
        wxSafeYield();  // Allows slice of time to redraw the screen
                        // to refresh widgets, before displaying messages
        HTML_MESSAGE_BOX mbox( this, _( "Messages" ) );
        mbox.ListSet( msg );
        mbox.ShowModal();
    }

    return true;
}
