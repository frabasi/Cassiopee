/*
 
 
 
              NUGA 
 
 
 
 */

#ifndef NUGA_H27_HXX
#define NUGA_H27_HXX

namespace NUGA
{
  class H27
  {
  public:
    template <typename crd_t>
    static void splitH27(crd_t& crd, E_Int* INT, E_Int* BOT, E_Int* TOP, E_Int* LEFT, E_Int* RIGHT, E_Int* FRONT, E_Int* BACK, E_Int* h271, E_Int* h272, E_Int* h273, E_Int* h274, E_Int* h275, E_Int* h276, E_Int* h277, E_Int* h278)
    {
      h271[0] = BOT[0]+1; h271[1] = INT[4]+1; h271[2] = LEFT[0]+1; h271[3] = INT[8]+1; h271[4] = FRONT[1]+1; h271[5] = INT[1]+1;
      h272[0] = BOT[1]+1; h272[1] = INT[5]+1; h272[2] = INT[8]+1; h272[3] = RIGHT[0]+1; h272[4] = FRONT[0]+1; h272[5] = INT[0]+1;
      h273[0] = BOT[2]+1; h273[1] = INT[6]+1; h273[2] = INT[9]+1; h273[3] = RIGHT[1]+1; h273[4] = INT[0]+1; h273[5] = BACK[0]+1;
      h274[0] = BOT[3]+1; h274[1] = INT[7]+1; h274[2] = LEFT[1]+1; h274[3] = INT[9]+1; h274[4] = INT[1]+1; h274[5] = BACK[1]+1;
      
      h275[0] = INT[4]+1; h275[1] = TOP[0]+1; h275[2] = LEFT[3]+1; h275[3] = INT[11]+1; h275[4] = FRONT[2]+1; h275[5] = INT[2]+1;
      h276[0] = INT[5]+1; h276[1] = TOP[1]+1; h276[2] = INT[11]+1; h276[3] = RIGHT[3]+1; h276[4] = FRONT[3]+1; h276[5] = INT[3]+1;
      h277[0] = INT[6]+1; h277[1] = TOP[2]+1; h277[2] = INT[10]+1; h277[3] = RIGHT[2]+1; h277[4] = INT[3]+1; h277[5] = BACK[3]+1;
      h278[0] = INT[7]+1; h278[1] = TOP[3]+1; h278[2] = LEFT[2]+1; h278[3] = INT[10]+1; h278[4] = INT[2]+1; h278[5] = BACK[2]+1;
    }
  template <typename crd_t>
    static void splitT10(crd_t& crd, E_Int* INT, E_Int* BOT, E_Int* F1, E_Int* F2, E_Int* F3, E_Int* h271, E_Int* h272, E_Int* h273, E_Int* h274, E_Int* h275, E_Int* h276, E_Int* h277, E_Int* h278, E_Int ndiag)
    {
      if (ndiag==1){
      h271[0] = BOT[0]+1; h271[1] = F1[0]+1; h271[2] = INT[0]+1; h271[3] = F3[1]+1;
      h272[0] = BOT[1]+1; h272[1] = F1[1]+1; h272[2] = F2[0]+1; h272[3] = INT[1]+1;
      h273[0] = BOT[2]+1; h273[1] = INT[2]+1; h273[2] = F2[1]+1; h273[3] = F3[0]+1;
      h274[0] = INT[3]+1; h274[1] = F2[2]+1; h274[2] = F3[2]+1; h274[3] = F1[2]+1; 
          
      h275[0] = INT[0]+1; h275[1] = INT[6]+1; h275[2] = INT[5]+1; h275[3] = F1[3]+1; 
      h276[0] = INT[1]+1; h276[1] = BOT[3]+1; h276[2] = INT[6]+1; h276[3] = INT[4]+1; 
      h277[0] = INT[2]+1; h277[1] = INT[4]+1; h277[2] = F2[3]+1; h277[3] = INT[7]+1; 
      h278[0] = INT[3]+1; h278[1] = INT[5]+1; h278[2] = F3[3]+1; h278[3] = INT[7]+1;
      }
      else if (ndiag==2) {
      h271[0] = BOT[0]+1; h271[1] = F1[0]+1; h271[2] = INT[0]+1; h271[3] = F3[1]+1;
      h272[0] = BOT[1]+1; h272[1] = F1[1]+1; h272[2] = F2[0]+1; h272[3] = INT[1]+1;
      h273[0] = BOT[2]+1; h273[1] = INT[2]+1; h273[2] = F2[1]+1; h273[3] = F3[0]+1;
      h274[0] = INT[3]+1; h274[1] = F2[2]+1; h274[2] = F3[2]+1; h274[3] = F1[2]+1; 
          
      h275[0] = INT[0]+1; h275[1] = BOT[3]+1; h275[2] = INT[5]+1; h275[3] = INT[6]+1; 
      h276[0] = INT[1]+1; h276[1] = INT[6]+1; h276[2] = F1[3]+1; h276[3] = INT[4]+1; 
      h277[0] = INT[2]+1; h277[1] = INT[5]+1; h277[2] = INT[7]+1; h277[3] = F3[3]+1; 
      h278[0] = INT[3]+1; h278[1] = INT[4]+1; h278[2] = INT[7]+1; h278[3] = F2[3]+1;
      }
      else {
      h271[0] = BOT[0]+1; h271[1] = F1[0]+1; h271[2] = INT[0]+1; h271[3] = F3[1]+1;
      h272[0] = BOT[1]+1; h272[1] = F1[1]+1; h272[2] = F2[0]+1; h272[3] = INT[1]+1;
      h273[0] = BOT[2]+1; h273[1] = INT[2]+1; h273[2] = F2[1]+1; h273[3] = F3[0]+1;
      h274[0] = INT[3]+1; h274[1] = F2[2]+1; h274[2] = F3[2]+1; h274[3] = F1[2]+1; 
          
      h275[0] = INT[0]+1; h275[1] = INT[4]+1; h275[2] = F3[3]+1; h275[3] = INT[7]+1; 
      h276[0] = INT[1]+1; h276[1] = INT[6]+1; h276[2] = INT[5]+1; h276[3] = F2[3]+1; 
      h277[0] = INT[2]+1; h277[1] = BOT[3]+1; h277[2] = INT[6]+1; h277[3] = INT[4]+1; 
      h278[0] = INT[3]+1; h278[1] = F1[3]+1; h278[2] = INT[7]+1; h278[3] = INT[5]+1;          
      }
    }

    template <typename crd_t>
    static void splitP13(crd_t& crd, E_Int* INT, E_Int* BOT, E_Int* F1, E_Int* F2, E_Int* F3, E_Int* F4, E_Int* h271, E_Int* h272, E_Int* h273, E_Int* h274, E_Int* h275, E_Int* h276, E_Int* h277, E_Int* h278, E_Int* h279, E_Int* h2710)
    {
      h271[0] = BOT[0]+1; h271[1] = F1[0]+1; h271[2] = INT[0]+1; h271[3] = INT[7]+1; h271[4] = F4[1]+1;
      h272[0] = BOT[1]+1; h272[1] = F1[1]+1; h272[2] = F2[0]+1; h272[3] = INT[2]+1; h272[4] = INT[1]+1;
      h273[0] = BOT[2]+1; h273[1] = INT[3]+1; h273[2] = F2[1]+1; h273[3] = F3[0]+1; h273[4] = INT[4]+1;
      h274[0] = BOT[3]+1; h274[1] = INT[6]+1; h274[2] = INT[5]+1; h274[3] = F3[1]+1; h274[4] = F4[0]+1;
      
      h277[0] = INT[8]+1; h277[1] = F1[3]+1; h277[2] = INT[0]+1; h277[3] = INT[1]+1;
      h278[0] = INT[9]+1; h278[1] = F2[3]+1; h278[2] = INT[2]+1; h278[3] = INT[3]+1; 
      h279[0] = INT[10]+1; h279[1] = F3[3]+1; h279[2] = INT[4]+1; h279[3] = INT[5]+1; 
      h2710[0] = INT[11]+1; h2710[1] = F4[3]+1; h2710[2] = INT[6]+1; h2710[3] = INT[7]+1; 
      
      h275[0] = INT[12]+1; h275[1] = F1[2]+1; h275[2] = F2[2]+1; h275[3] = F3[2]+1; h275[4] = F4[2]+1;
      h276[0] = INT[12]+1; h276[1] = INT[11]+1; h276[2] = INT[10]+1; h276[3] = INT[9]+1; h276[4] = INT[8]+1;
    }  
    
    template <typename crd_t>
    static void splitPr18(crd_t& crd, E_Int* INT, E_Int* BOT, E_Int* F1, E_Int* F2, E_Int* F3, E_Int* TOP, E_Int* h271, E_Int* h272, E_Int* h273, E_Int* h274, E_Int* h275, E_Int* h276, E_Int* h277, E_Int* h278)
    {
      h271[0] = BOT[0]+1; h271[1] = F1[0]+1; h271[2] = INT[4]+1; h271[3] = F3[1]+1; h271[4] = INT[0]+1;
      h272[0] = BOT[1]+1; h272[1] = F1[1]+1; h272[2] = F2[0]+1; h272[3] = INT[5]+1; h272[4] = INT[1]+1;
      h273[0] = BOT[2]+1; h273[1] = INT[6]+1; h273[2] = F2[1]+1; h273[3] = F3[0]+1; h273[4] = INT[2]+1;
      h274[0] = INT[3]+1; h274[1] = INT[4]+1; h274[2] = INT[6]+1; h274[3] = INT[5]+1; h274[4] = BOT[3]+1;
 
      h275[0] = INT[0]+1; h275[1] = F1[3]+1; h275[2] = INT[7]+1; h275[3] = F3[2]+1; h275[4] = TOP[0]+1;
      h276[0] = INT[1]+1; h276[1] = F1[2]+1; h276[2] = F2[3]+1; h276[3] = INT[8]+1; h276[4] = TOP[1]+1;
      h277[0] = INT[2]+1; h277[1] = INT[9]+1; h277[2] = F2[2]+1; h277[3] = F3[3]+1; h277[4] = TOP[2]+1;
      h278[0] = INT[3]+1; h278[1] = INT[8]+1; h278[2] = INT[9]+1; h278[3] = INT[7]+1; h278[4] = TOP[3]+1; 
    } 
        
   };
}

#endif
