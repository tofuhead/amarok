#include <iostream>
#include <string>
#include <dlfcn.h>
#include <sys/param.h>

#include <qthread.h>
#include <qmutex.h>

#include "hxsplay.h"

using namespace std;

HXSplay::PlayerPkg::PlayerPkg(HXSplay *player, int playerIndex) : 
   m_player(player),
   m_playerIndex(playerIndex),
   m_playthreadid(0),
   m_pt_state(HXSplay::STOP)
{
}


HXSplay::HXSplay(int &xflength) : handle(0),m_Err(0),m_numPlayers(0), m_xfadeLength(xflength)
{
}

HXSplay::~HXSplay()
{
   int i;
   for (i=0; i<m_numPlayers; i++)
      stop(i);
}


void HXSplay::init(const char *corelibpath, 
                   const char *pluginslibpath, 
                   const char *codecspath)
{
   m_Err =0;

   if (!m_Err)
   {
      m_numPlayers = 2;
      m_current = 1;
      HelixSimplePlayer::init(corelibpath, pluginslibpath, codecspath, 2);
      
      int i;
      
      for (i=0; i<m_numPlayers; i++)
         m_playerPkg[i] = new PlayerPkg(this, i);
   }
}

int HXSplay::addPlayer()
{
   if (!m_Err)
   {
      if (m_numPlayers < MAX_PLAYERS)
      {
         m_playerPkg[m_numPlayers] = new PlayerPkg(this, m_numPlayers);
         m_numPlayers++;
         return HelixSimplePlayer::addPlayer();
      }
   }

   return -1;
}

void HXSplay::PlayerPkg::run()
{
   HXSplay *splay = m_player;
   int playerIndex = m_playerIndex;
   HXSplay::pthr_states state = HXSplay::PLAY;

   cerr << "Play thread started for player " << playerIndex << endl;
   splay->startPlayer(playerIndex);
   while (!splay->done(playerIndex))
   {
      m_ptm.lock();  // this mutex is probably overkill, since we only read state
      switch (m_pt_state)
      {
         case HXSplay::STOP:
            if (state != HXSplay::STOP) 
               splay->stopPlayer(playerIndex);
            cerr << "Play thread was stopped for player " << playerIndex << endl;
            m_ptm.unlock();
            return;

         case HXSplay::PAUSE:
            if (state == HXSplay::PLAY)
               splay->pausePlayer(playerIndex);
            state = HXSplay::PAUSE;
            break;

         case HXSplay::PLAY:
            if (state == HXSplay::PAUSE)
               splay->resumePlayer(playerIndex);
            state = HXSplay::PLAY;
            break;
      }
      m_ptm.unlock();
      splay->dispatch();
   } 
   m_ptm.lock();
   splay->stopPlayer(playerIndex);
   m_pt_state = HXSplay::STOP;
   m_ptm.unlock();

   splay->play_finished(playerIndex);
   cerr << "Play thread finished for player " << playerIndex << endl;
   return;
}


void HXSplay::play()
{
   PlayerPkg *pkg;
   int nextPlayer;

   cerr << "HXXFadeLength " << m_xfadeLength << endl;

   nextPlayer = m_current ? 0 : 1;
   pkg = m_playerPkg[nextPlayer];

   pkg->m_ptm.lock();
   if (!m_Err && pkg->m_pt_state == HXSplay::STOP )
   {
      pkg->m_pt_state = HXSplay::PLAY;
      pkg->start();
   }
   pkg->m_ptm.unlock();

   m_current = nextPlayer;
}

HXSplay::pthr_states HXSplay::state() const
{
   return m_playerPkg[m_current]->m_pt_state;
}

void HXSplay::stop()
{
   PlayerPkg *pkg;

   cerr << "In STOP " << m_current << endl;

   pkg = m_playerPkg[m_current];
   HXSplay::pthr_states state;

   pkg->m_ptm.lock();
   state = pkg->m_pt_state;
   pkg->m_pt_state = STOP;
   pkg->m_ptm.unlock();

   if (state != HXSplay::STOP)
      pkg->wait();
}

void HXSplay::stop(int playerIndex)
{
   PlayerPkg *pkg;

   pkg = m_playerPkg[playerIndex];
   HXSplay::pthr_states state;

   pkg->m_ptm.lock();
   state = pkg->m_pt_state;
   pkg->m_pt_state = STOP;
   pkg->m_ptm.unlock();

   if (state != HXSplay::STOP)
      pkg->wait();   
}

void HXSplay::pause()
{
   PlayerPkg *pkg;

   pkg = m_playerPkg[m_current];

   pkg->m_ptm.lock();
   if (pkg->m_pt_state == HXSplay::PLAY)
      pkg->m_pt_state = HXSplay::PAUSE;
   pkg->m_ptm.unlock();
}

void HXSplay::resume()
{
   PlayerPkg *pkg;

   pkg = m_playerPkg[m_current];

   pkg->m_ptm.lock();
   if (pkg->m_pt_state == HXSplay::PAUSE)
      pkg->m_pt_state = HXSplay::PLAY;
   pkg->m_ptm.unlock();
}

unsigned long HXSplay::where() const
{
   return HelixSimplePlayer::where(m_current);
}

unsigned long HXSplay::duration() const
{
   return HelixSimplePlayer::duration(m_current);
}

void HXSplay::seek(unsigned long ms)
{
   HelixSimplePlayer::seek(ms, m_current);
}

int HXSplay::setURL(const char *file)
{
   return HelixSimplePlayer::setURL(file, m_current ? 0 : 1); // we only actually flip the player if we actually play
}


////////////////////////////////////////////////////////////////
void HXSplay::startPlayer(int playerIndex)
{
   HelixSimplePlayer::start(playerIndex, false, false, 30000);
}

void HXSplay::stopPlayer(int playerIndex)
{
   HelixSimplePlayer::stop(playerIndex);
}

void HXSplay::pausePlayer(int playerIndex)
{
   HelixSimplePlayer::pause(playerIndex);
}

void HXSplay::resumePlayer(int playerIndex)
{
   HelixSimplePlayer::resume(playerIndex);
}


//#include "hxsplay.moc"
