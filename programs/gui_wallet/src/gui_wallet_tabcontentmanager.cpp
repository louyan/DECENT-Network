/* (c) 2016, 2017 DECENT Services. For details refers to LICENSE.txt */
#include "gui_wallet_global.hpp"
#include "gui_wallet_tabcontentmanager.hpp"

#include <QTimer>

using std::string;

namespace gui_wallet
{
TabContentManager::TabContentManager(QWidget* pParent/* = nullptr*/) : QWidget(pParent)
, m_i_page_size(50)
, m_last_result()
, m_next_iterator()
, m_pFilterWidget(nullptr)
, m_pRefreshTimer(nullptr)
{
}

void TabContentManager::tryToUpdate() {
   try {
      std::string command = getUpdateCommand();
      if (command.empty()) {
         timeToUpdate(std::string());
         return;
      }

      std::string result, error;
      try {
         result = Globals::instance().runTask(command);
      }
      catch (std::exception &ex) {
         error = ex.what();

      }
      catch (fc::exception& ex) {
         error = ex.what();

      }

      if (result != m_last_result) {
         m_last_result = result;
         timeToUpdate(result);
      }
   } catch (...) {

   }
}

bool TabContentManager::next()
{
   if (is_last())
      return false;

   m_iterators.push_back(m_next_iterator);
   tryToUpdate();
   return true;
}

void TabContentManager::reset(bool bRefresh/* = true*/)
{
   m_iterators.clear();
   m_next_iterator.clear();

   if (bRefresh)
   {
      m_last_result.clear();
      tryToUpdate();
   }
}

bool TabContentManager::previous()
{
   if (is_first())
      return false;

   m_iterators.pop_back();
   tryToUpdate();
   return true;
}

bool TabContentManager::is_first() const
{
   return m_iterators.empty();
}

bool TabContentManager::is_last() const
{
   return m_next_iterator.empty();
}

void TabContentManager::set_next_page_iterator(string const& iterator)
{
   m_next_iterator = iterator;
}

std::string TabContentManager::next_iterator() const
{
   string str_iterator;
   if (!m_iterators.empty())
      str_iterator = m_iterators.back();

   return str_iterator;
}

void TabContentManager::setFilterWidget(QWidget* pWidget)
{
   m_pFilterWidget = pWidget;
}

void TabContentManager::setRefreshTimer(int msec)
{
   if (m_pRefreshTimer == nullptr) {
      m_pRefreshTimer = new QTimer(this);
      m_pRefreshTimer->setInterval(msec);
   }
   else {
      m_pRefreshTimer->setInterval(msec);
   }
}


}  // namespace gui_wallet
