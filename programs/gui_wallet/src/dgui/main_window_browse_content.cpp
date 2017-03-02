// main_window_browse_content
/*
 *	File: main_window_browse_content.cpp
 *
 *	Created on: 20 Feb 2017
 *	Created by:
 *
 *  This file implements ...
 *
 */

#include "gui_wallet_mainwindow.hpp"
#include <fc/variant_object.hpp>
#include "decent_wallet_ui_gui_jsonparserqt.hpp"


/*///////////////////////////////////////////////////////*/

static int s_nActive = 0;
void ParseDigitalContentFromVariant(decent::wallet::ui::gui::SDigitalContent* a_pContent,
                                    const fc::variant& a_result);

void gui_wallet::Mainwindow_gui_wallet::ManagementBrowseContentGUI()
{
    std::string ssTask;
    QString cqsNewFilter = m_pCentralWidget->getFilterText();

    const char* cpcFilter = strchr(cqsNewFilter.toStdString().c_str(),':');

    if(!cpcFilter++){cpcFilter = "";}

    if(strstr(cqsNewFilter.toStdString().c_str(),"author"))
    {
        ssTask = std::string("list_content_by_author ") + cpcFilter;
    }
    else
    {
        if(strlen(cpcFilter)==0)
        {
            ssTask = "list_content a 100";
        }
        else
        {
            const char* cpcFilter0(cpcFilter);
            for(;(*cpcFilter)&&(*cpcFilter != ' ');++cpcFilter);
            if(atoi(cpcFilter)){ssTask = std::string("list_content ") + cpcFilter0;}
            else{ssTask = std::string("list_content ") + cpcFilter0 + " 100";}
        }
    }

    if(s_nActive==0)
    {
        s_nActive = 1;
//#if DEFAULT_LOG_LEVEL==0
        SetNewTask3(ssTask,this,NULL,&Mainwindow_gui_wallet::TaskDoneBrowseContentGUI3);
//#endif
    }

}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneBrowseContentGUI(void* a_clbkArg,int64_t a_err,
                                                                 const std::string& a_task,const std::string& a_result)
{
    //
}


void gui_wallet::Mainwindow_gui_wallet::TaskDoneBrowseContentGUI3(void* a_clbkArg,int64_t a_err,
                                                                  const std::string& a_task,
                                                                  const fc::variant& a_result)
{
    decent::wallet::ui::gui::JsonParserQt aVisitor;
    aVisitor.m_inp = a_task;
    __DEBUG_APP2__(2," ");

    if(a_err){return;}

    a_result.visit(aVisitor);
    //aVisitor.PrintValues();
    //const decent::wallet::ui::gui::JsonParserQt& visRes = aVisitor.GetByIndex(0).GetByKey("author");
    //std::string aRes = visRes.value();
    //printf("!!!!!!!!!!!!!!!! type=%s, val=%s\n",visRes.TypeToString(), aRes.c_str());
    //printf("!!!!!!!!!!!!!!!!!!!!!!!!!! g_nCreateAndDelete=%d\n",g_nCreateAndDelete);

    if(strstr(a_task.c_str(),"list_content ") || strstr(a_task.c_str(),"list_content_by_author "))
    {
        std::string aNewTask;
        m_vcDigContent.clear();
        GetDigitalContentsFromVariant(DCT::GENERAL,m_vcDigContent,a_result);
        int nSize(m_vcDigContent.size());
        for(int i(0);i<nSize;++i)
        {
            aNewTask = "get_content \"" + m_vcDigContent[i].URI + "\"";
            SetNewTask3(aNewTask,this,(void*)((size_t)i),&gui_wallet::Mainwindow_gui_wallet::TaskDoneBrowseContentGUI3);
        }
    }
    else if(strstr(a_task.c_str(),"get_content "))
    {
        const int cnIndex (  (int)(  (size_t)a_clbkArg  )     );
        const int cnContsNumber(m_vcDigContent.size());
        if(cnIndex>=cnContsNumber){return;}
        ParseDigitalContentFromVariant(&m_vcDigContent[cnIndex],a_result);
        if(cnIndex==(cnContsNumber-1))
        {
            bool bToDraw(true);
            if(cnContsNumber == m_vcDigContentOld.size())
            {
                bToDraw = false;
                for(int i(0); i<cnContsNumber;++i)
                {
                    if(m_vcDigContentOld[i].URI != m_vcDigContent[i].URI)
                    {
                        bToDraw = true;
                        break;
                    }
                }  // for(int i(0); i<cnContsNumber;++i)
            }
            if(bToDraw){m_pCentralWidget->SetDigitalContentsGUI(m_vcDigContent);m_vcDigContentOld = m_vcDigContent;}
        }

    }

    s_nActive = 0;
    //std::cout<<"brows"<<std::endl;
    //m_pCentralWidget->m_browse_cont_tab.Connects();
}



/*////////////////////////////////////////////////////*/

//void FinalResult::Clear2(){m_values.clear();}
