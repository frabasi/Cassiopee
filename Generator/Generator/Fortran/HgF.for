C  
C    Copyright 2013-2021 Onera.
C
C    This file is part of Cassiopee.
C
C    Cassiopee is free software: you can redistribute it and/or modify
C    it under the terms of the GNU General Public License as published by
C    the Free Software Foundation, either version 3 of the License, or
C    (at your option) any later version.
C
C    Cassiopee is distributed in the hope that it will be useful,
C    but WITHOUT ANY WARRANTY; without even the implied warranty of
C    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
C    GNU General Public License for more details.
C
C    You should have received a copy of the GNU General Public License
C    along with Cassiopee.  If not, see <http://www.gnu.org/licenses/>.

C ============================================================================
C 2D hyperbolic mesh generator
C  ===========================================================================
      SUBROUTINE k6hyper2D(ni, nj, d, xi, yi, zi,
     &                   type, 
     &                   xd, yd, zd,
     &                   IP, A, B, C, RHS, Z, ZA, vol,
     &                   eta_start, eta_end, betas,
     &                   eta_start2, eta_end2, beta2s)

/* #define MODIFIED_VOLUME(x) x		*/
#define MODIFIED_VOLUME(x) SQRT(g11)*x
C
      IMPLICIT NONE
C==============================================================================
C_IN
      INTEGER_E ni		   ! nbre de points sur une ligne eta=cte
      INTEGER_E nj		   ! nbre de points sur une ligne xi=cte
      REAL_E d(*)                ! distribution 2D
      REAL_E xi(*),yi(*),zi(*)   ! Ligne 1D
      INTEGER_E type             ! 0 = C, 1 = O.
C_OUT
      REAL_E xd(*), yd(*), zd(*)   ! Maillage final
C_LOCAL
      REAL_E dxdxi		!derivees
      REAL_E dydxi
      REAL_E dxdeta
      REAL_E dydeta
      INTEGER_E IP(2,ni-1)	        !tableau de travail (reste de f77)
      REAL_E A(2,2,ni)	        !pour resolution du systeme
      REAL_E B(2,2,ni)	        !tridiagonal par blocs
      REAL_E C(2,2,ni)
      REAL_E RHS(2,ni)
      REAL_E Z(2,2,ni-1)
      REAL_E ZA(2,ni-2)
      REAL_E vol(ni*nj)
      INTEGER_E i,j,k,IER,ni1,i1,nk ! prive
      REAL_E g11,b1,b2,ba1,ba2,beta
      REAL_E pi,sin_teta1,cos_teta1,norm,sin_teta2,cos_teta2
      INTEGER_E indice,ind,indp1,indm1,indp2,indm2,indb
      INTEGER_E eta_start,eta_end,indv
      INTEGER_E eta_start2,eta_end2
      REAL_E betas,beta2s
      REAL_E cca,ccb,cca2,ccb2,beta2
      INTEGER_E deux
C==============================================================================
      indice(i,j) = i+(j-1)*ni

      pi = 4*atan(1.D0)
      deux = 2

C Anciens reglages par defaut     
C      IF (type.EQ.0) THEN
C       Parametres de la dissipation pour les maillages en C
C        eta_start = 1
C        eta_end = nj
C        eta_start2 = 50
C        eta_end2 = nj
C       Dissipation d'ordre 4 (<0.125)
C        betas = 0.1D0
C        beta = -betas
C       Dissipation d'ordre 2
C        beta2s = 0.D0
C        beta2 = -beta2s
C      ELSE
C       Parametres de la dissipation pour les maillages en O
C        eta_start = 1
C        eta_end = nj
C        eta_start2 = 3
C        eta_end2 = nj
C       Dissipation d'ordre 4 (<0.125)
C        betas = 0.D0
C        beta = -betas
C       Dissipation d'ordre 2
C        beta2s = 0.D0
C        beta2 = -beta2s
C      ENDIF

C gestion des bornes negatives
      IF (eta_end.LT.0) THEN
        eta_end = nj-eta_end+1
      ENDIF
      IF (eta_end2.LT.0) THEN
        eta_end2 = nj-eta_end2+1
      ENDIF

C calcul des constantes des rampes
      IF (eta_end.EQ.eta_start) THEN
        cca = 1.D0
      ELSE
        cca = -betas/(eta_end-eta_start)
      ENDIF
      IF (eta_end2.EQ.eta_start2) THEN
        cca2 = 1.D0
      ELSE
        cca2 = -beta2s/(eta_end2-eta_start2)
      ENDIF

      ccb  = -betas-cca*eta_end
      ccb2 = -beta2s-cca2*eta_end2

      IF (type.EQ.0) THEN
        nk = ni
      ELSE IF (type.EQ.1) THEN
        nk = ni
      ELSE
        WRITE(*,*) 'hg: unknown type of mesh.'
        RETURN
      ENDIF

C*--------------------------------initialisations----------------------------*C
C note: delta_xi=1 et delta_eta=1

C* Initialisation
      DO i = 1, ni
        DO j = 1, nj-1
          indv = i + (j-1)*ni
          vol(indv) = d(i+j*ni) - d(i+(j-1)*ni)
        ENDDO
      ENDDO

      DO i = 1, ni
        ind = indice(i,1)
        xd(ind) = xi(i)
        yd(ind) = yi(i)
      ENDDO
      DO i = 1, ni*nj
        zd(i) = 0.D0
      ENDDO

      IF (type.EQ.1) THEN
        ind = indice(ni,1)
        indb = indice(1,1)
        xd(ind) = xd(indb)
        yd(ind) = yd(indb)
      ENDIF

C*
C* Schema implicite
C*
      ind = indice(1,1)
      indp1 = indice(2,1)
      norm = SQRT((yd(indp1)-yd(ind))**2+(xd(indp1)-xd(ind))**2)
      sin_teta1 = (yd(indp1)-yd(ind))/norm
      cos_teta1 = (xd(indp1)-xd(ind))/norm
    
      ind = indice(ni,1)
      indm1 = indice(ni-1,1)
      norm = SQRT((yd(ind)-yd(indm1))**2+(xd(ind)-xd(indm1))**2)
      sin_teta2 = (yd(indm1)-yd(ind))/norm
      cos_teta2 = (xd(indm1)-xd(ind))/norm
    
      DO j = 1, nj-1
C	    WRITE(*,*) '->Computing plane :',j
       
C* dissipation variant lineairement

        IF (j.LT.eta_start) THEN
          beta = 0.D0
        ELSE IF (j.GT.eta_end) THEN
          beta = -betas
        ELSE
          beta = cca*j+ccb
        ENDIF
        IF (j.LT.eta_start2) THEN
          beta2 = 0.D0
        ELSE IF (j.GT.eta_end2) THEN
          beta2 = -beta2s
        ELSE
          beta2 = cca2*j+ccb2
        ENDIF
        
C* Modification pour coupure non verticale
C* teta=0 <=> coupure verticale
    
        IF (type.EQ.0) THEN	! conditions aux limites type C
                ! dirichlet en x, neuman en y
            
        DO i = 3, ni-2
         indv = i+(j-1)*ni
         indp1 = indice(i+1,j)
         indm1 = indice(i-1,j)
         dxdxi = (xd(indp1)-xd(indm1))*0.5D0
         dydxi = (yd(indp1)-yd(indm1))*0.5D0
         g11 = dxdxi*dxdxi+dydxi*dydxi
         vol(indv) = MODIFIED_VOLUME(vol(indv))
         dxdeta = -vol(indv)*dydxi/g11
         dydeta =  vol(indv)*dxdxi/g11
         
         b1 = dxdxi/g11
         b2 = dydxi/g11
         ba1 = b1*dxdeta
         ba2 = (b1*dydeta+b2*dxdeta)*0.5D0
         i1 = i-1		
         
         B(1,1,i1) = ba1+beta2
         B(2,1,i1) = ba2
         B(1,2,i1) = ba2
         B(2,2,i1) = -ba1+beta2
         
         C(1,1,i1) = -ba1+beta2
         C(2,1,i1) = -ba2
         C(1,2,i1) = -ba2
         C(2,2,i1) = ba1+beta2
         
         A(1,1,i1) = 1.D0-2*beta2
         A(2,1,i1) = 0.D0
         A(1,2,i1) = 0.D0
         A(2,2,i1) = 1.D0-2*beta2
         
         indp2 = indice(i+2,j)
         indp1 = indice(i+1,j)
         ind = indice(i,j)
         indm1 = indice(i-1,j)
         indm2 = indice(i-2,j)
         
         RHS(1,i1)=-2.D0*vol(indv)*b2+xd(ind)+beta*(xd(indm2)-
     &              4.D0*xd(indm1)+6.D0*xd(ind)-4.D0*xd(indp1)
     &              +xd(indp2))
         RHS(2,i1)= 2.D0*vol(indv)*b1+yd(ind)+beta*(yd(indm2)-
     &              4.D0*yd(indm1)+6.D0*yd(ind)-4.D0*yd(indp1)
     &              +yd(indp2))
        
        ENDDO
        
c i=2
        indp1 = indice(3,j) 
        indm1 = indice(1,j)
        dxdxi = (xd(indp1)-xd(indm1))*0.5D0
        dydxi = (yd(indp1)-yd(indm1))*0.5D0
        g11 = dxdxi*dxdxi+dydxi*dydxi
        indv = 2+(j-1)*ni
        vol(indv) = MODIFIED_VOLUME(vol(indv))
                
        dxdeta = -vol(indv)*dydxi/g11
        dydeta = vol(indv)*dxdxi/g11
        b1 = dxdxi/g11		
        b2 = dydxi/g11
        ba1 = b1*dxdeta
        ba2 = (b1*dydeta+b2*dxdeta)*0.5D0
        
        B(1,1,1)=ba1+(1.D0/3.D0)*sin_teta1*sin_teta1*ba1- 
     &		(1./3.)*sin_teta1*cos_teta1*ba2
        B(2,1,1)=ba2+(1.D0/3.D0)*sin_teta1*sin_teta1*ba2+ 
     &		(1./3.)*sin_teta1*cos_teta1*ba1
        B(1,2,1)=ba2-(1.D0/3.D0)*sin_teta1*cos_teta1*ba1+ 
     &		(1./3.)*cos_teta1*cos_teta1*ba2
        B(2,2,1)=-ba1-(1.D0/3.D0)*cos_teta1*cos_teta1*ba1- 
     &		(1./3.)*cos_teta1*sin_teta1*ba2
        
        C(1,1,1) = 0.D0
        C(1,2,1) = 0.D0
        C(2,1,1) = 0.D0
        C(2,2,1) = 0.D0
        
        A(1,1,1)=1.-(4.D0/3.D0)*sin_teta1*sin_teta1*ba1+ 
     &		(4.D0/3.D0)*sin_teta1*cos_teta1*ba2
        A(2,1,1)=-(4.D0/3.D0)*sin_teta1*sin_teta1*ba2- 
     &		(4.D0/3.D0)*sin_teta1*cos_teta1*ba1
        A(1,2,1)=(4.D0/3.D0)*sin_teta1*cos_teta1*ba1- 
     &		(4./3.)*cos_teta1*cos_teta1*ba2
        A(2,2,1)=1.+(4.D0/3.D0)*sin_teta1*cos_teta1*ba2+ 
     &		(4.D0/3.D0)*cos_teta1*cos_teta1*ba1
        
C les CL sur la dissipation sont de type eriksson-dissip explicite
        indp2 = indice(4,j)
        indp1 = indice(3,j)
        ind = indice(2,j)
        indm1 = indice(1,j)
        indb = indice(1,1)
        
        RHS(1,1)=-2.D0*vol(indv)*b2+xd(ind)+beta*
     &      (-xd(indm1)+3.D0*xd(ind) 
     &		-3.D0*xd(indp1)+ xd(indp2))+ 
     &		ba1*cos_teta1*cos_teta1*xd(indb)+ 
     &		ba2*sin_teta1*sin_teta1*yd(indb)+ 
     &		ba1*sin_teta1*cos_teta1*yd(indb)+ 
     &		ba2*sin_teta1*cos_teta1*xd(indb)
        
        RHS(2,1)= 2.D0*vol(indv)*b1+yd(ind)+beta*
     &      (-yd(indm1)+3.D0*yd(ind) 
     &  	-3.D0*yd(indp1)+ yd(indp2))+ 
     &		ba2*cos_teta1*cos_teta1*xd(indb)+ 
     &		ba2*cos_teta1*sin_teta1*yd(indb)- 
     &		ba1*cos_teta1*sin_teta1*xd(indb)- 
     &		ba1*sin_teta1*sin_teta1*yd(indb)
    
C i = ni-1
        ni1 = ni-1
        indp1 = indice(ni,j)
        indm1 = indice(ni-2,j)
        dxdxi = (xd(indp1)-xd(indm1))*0.5D0
        dydxi = (yd(indp1)-yd(indm1))*0.5D0
        g11 = dxdxi*dxdxi+dydxi*dydxi 
        indv = ni1+(j-1)*ni
        vol(indv) = MODIFIED_VOLUME(vol(indv))
                
        dxdeta = -vol(indv)*dydxi/g11
        dydeta = vol(indv)*dxdxi/g11
        
        b1 = dxdxi/g11
        b2 = dydxi/g11
        ba1 = b1*dxdeta
        ba2 = (b1*dydeta+b2*dxdeta)*0.5D0
        
        B(1,1,ni1-1) = 0.D0
        B(2,1,ni1-1) = 0.D0
        B(1,2,ni1-1) = 0.D0
        B(2,2,ni1-1) = 0.D0
        
        C(1,1,ni1-1)=-ba1-(1.D0/3.D0)*sin_teta2*sin_teta2*ba1+ 
     &		(1.D0/3.D0)*sin_teta2*cos_teta2*ba2
        C(2,1,ni1-1)=-ba2-(1.D0/3.D0)*sin_teta2*sin_teta2*ba2- 
     &		(1.D0/3.D0)*sin_teta2*cos_teta2*ba1
        C(1,2,ni1-1)=-ba2+(1.D0/3.D0)*sin_teta2*cos_teta2*ba1- 
     &		(1.D0/3.D0)*cos_teta2*cos_teta2*ba2
        C(2,2,ni1-1)= ba1+(1.D0/3.D0)*sin_teta2*cos_teta2*ba2+ 
     &		(1.D0/3.D0)*cos_teta2*cos_teta2*ba1
        
        A(1,1,ni1-1)=1.+(4.D0/3.D0)*sin_teta2*sin_teta2*ba1- 
     &		(4.D0/3.D0)*sin_teta2*cos_teta2*ba2	
        A(2,1,ni1-1)=(4.D0/3.D0)*sin_teta2*sin_teta2*ba2+ 
     &		(4.D0/3.D0)*sin_teta2*cos_teta2*ba1
        A(1,2,ni1-1)=-(4.D0/3.D0)*sin_teta2*cos_teta2*ba1+ 
     &		(4.D0/3.D0)*cos_teta2*cos_teta2*ba2
        A(2,2,ni1-1)=1.-(4.D0/3.D0)*sin_teta2*cos_teta2*ba2- 
     &		(4.D0/3.D0)*cos_teta2*cos_teta2*ba1
        
        indp1 = indice(ni,j)
        ind = indice(ni-1,j)
        indm1 = indice(ni-2,j)
        indm2 = indice(ni-3,j)
        indb = indice(ni,1)
    
        RHS(1,ni1-1)=-2.D0*vol(indv)*b2+xd(ind)+beta*(-xd(indp1)+ 
     &    	     3.D0*xd(ind)- 3.D0*xd(indm1)+xd(indm2))- 
     &	             ba1*cos_teta2*cos_teta2*xd(indb)- 
     &		     ba2*sin_teta2*cos_teta2*xd(indb)- 
     &		     ba2*sin_teta2*sin_teta2*yd(indb)- 
     &		     ba1*cos_teta2*sin_teta2*yd(indb)		
        
        RHS(2,ni1-1)= 2.D0*vol(indv)*b1+yd(ind)+beta*(-yd(indp1)+ 
     &	             3.D0*yd(ind)- 3.D0*yd(indm1)+yd(indm2))- 
     &	             ba2*cos_teta2*cos_teta2*xd(indb)- 
     &	             ba2*sin_teta2*cos_teta2*yd(indb)+ 
     &	             ba1*sin_teta2*cos_teta2*xd(indb)+ 
     &	             ba1*sin_teta2*sin_teta2*yd(indb)
    
C* Inversion
        
C        DO i = 1, ni-2
C            WRITE(*,*) 'A',A(1,1,i),A(1,2,i)
C            WRITE(*,*) 'A',A(2,1,i),A(2,2,i)
C            WRITE(*,*) 'B',B(1,1,i),B(1,2,i)
C            WRITE(*,*) 'B',B(2,1,i),B(2,2,i)
C            WRITE(*,*) 'C',C(1,1,i),C(1,2,i)
C            WRITE(*,*) 'C',C(2,1,i),C(2,2,i)
C            WRITE(*,*) 'RHS',RHS(1,i),RHS(2,i)            
C        ENDDO

        CALL k6DECBT(deux,ni-2,A,B,C,IP,IER)
        IF (IER.NE.0) THEN
            WRITE(*,*) 'IER error:',IER
            STOP 'MATRICE SINGULIERE'
        ENDIF
          
        CALL k6SOLBT(deux,ni-2,A,B,C,RHS,IP)
          
        DO i = 2, ni-1
         ind = indice(i,j+1)
         xd(ind) = RHS(1,i-1)
         yd(ind) = RHS(2,i-1)
        ENDDO
    
C* i=1	
        ind = indice(1,j+1)
        indp1 = indice(2,j+1)
        indp2 = indice(3,j+1)
        indb = indice(1,1)
    
        xd(ind)=-(4.D0/3.D0)*sin_teta1*cos_teta1*yd(indp1)+ 
     &	               (1.D0/3.D0)*sin_teta1*cos_teta1*yd(indp2)+ 
     &	               (4.D0/3.D0)*sin_teta1*sin_teta1*xd(indp1)- 
     &	               (1.D0/3.D0)*sin_teta1*sin_teta1*xd(indp2)+ 
     &	       cos_teta1*cos_teta1*xd(indb)+sin_teta1*cos_teta1*yd(indb)
        
        yd(ind)=sin_teta1*cos_teta1*xd(indb)+ 
     &	               sin_teta1*sin_teta1*yd(indb)+ 
     &	              (4.D0/3.D0)*cos_teta1*cos_teta1*yd(indp1)- 
     &	              (1.D0/3.D0)*cos_teta1*cos_teta1*yd(indp2)- 
     &	              (4.D0/3.D0)*sin_teta1*cos_teta1*xd(indp1)+ 
     &	              (1.D0/3.D0)*sin_teta1*cos_teta1*xd(indp2)
    
C* i=ni	
        ind = indice(ni,j+1)
        indm1 = indice(ni-1,j+1)
        indm2 = indice(ni-2,j+1)
        indb = indice(ni,1)
            
        xd(ind)=-(4.D0/3.D0)*sin_teta2*cos_teta2*yd(indm1)+ 
     &	            (1.D0/3.D0)*sin_teta2*cos_teta2*yd(indm2)+ 
     &	            (4.D0/3.D0)*sin_teta2*sin_teta2*xd(indm1)- 
     &	            (1.D0/3.D0)*sin_teta2*sin_teta2*xd(indm2)+ 
     &	    cos_teta2*cos_teta2*xd(indb)+sin_teta2*cos_teta2*yd(indb)
        
        yd(ind)=sin_teta2*cos_teta2*xd(indb)+ 
     &	          sin_teta2*sin_teta2*yd(indb)+ 
     &	          (4.D0/3.D0)*cos_teta2*cos_teta2*yd(indm1)- 
     &	          (1.D0/3.D0)*cos_teta2*cos_teta2*yd(indm2)- 
     &	          (4.D0/3.D0)*sin_teta2*cos_teta2*xd(indm1)+ 
     &	          (1.D0/3.D0)*sin_teta2*cos_teta2*xd(indm2)
    
      ENDIF
    
C*------*-------*-------*------------*-------------
    
      IF (type.EQ.1) THEN	!condition aux limites en O
        
        DO i = 3, ni-2
          indv = i+(j-1)*ni
          indp1 = indice(i+1,j)
          indm1 = indice(i-1,j)
          dxdxi = (xd(indp1)-xd(indm1))*0.5D0
          dydxi = (yd(indp1)-yd(indm1))*0.5D0
          g11 = dxdxi*dxdxi+dydxi*dydxi
          vol(indv) = MODIFIED_VOLUME(vol(indv))
          dxdeta = -vol(indv)*dydxi/g11
          dydeta = vol(indv)*dxdxi/g11
                
          b1 = dxdxi/g11
          b2 = dydxi/g11
          ba1 = b1*dxdeta
          ba2 = (b1*dydeta+b2*dxdeta)*0.5D0
          i1 = i-1
          
          B(1,1,i1) = ba1+beta2
          B(2,1,i1) = ba2
          B(1,2,i1) = ba2
          B(2,2,i1) = -ba1+beta2
          
          C(1,1,i1) = -ba1+beta2
          C(2,1,i1) = -ba2
          C(1,2,i1) = -ba2
          C(2,2,i1) = ba1+beta2
          
          A(1,1,i1) = 1.D0-2*beta2
          A(2,1,i1) = 0.D0
          A(1,2,i1) = 0.D0
          A(2,2,i1) = 1.D0-2*beta2

          indp2 = indice(i+2,j)
          indp1 = indice(i+1,j)
          ind = indice(i,j)
          indm1 = indice(i-1,j)
          indm2 = indice(i-2,j)
                
          RHS(1,i1)=-2.D0*vol(indv)*b2+xd(ind)+beta*(xd(indm2)- 
     &        4.D0*xd(indm1)+ 6.D0*xd(ind)-4.D0*xd(indp1)+xd(indp2))
          RHS(2,i1)= 2.D0*vol(indv)*b1+yd(ind)+beta*(yd(indm2)- 
     &	      4.D0*yd(indm1)+ 6.D0*yd(ind)-4.D0*yd(indp1)+yd(indp2))

        ENDDO
C* i=2
       indv = 2+(j-1)*ni
       indp1 = indice(3,j)
       indm1 = indice(1,j)
       dxdxi = (xd(indp1)-xd(indm1))*0.5D0
       dydxi = (yd(indp1)-yd(indm1))*0.5D0
       g11 = dxdxi*dxdxi+dydxi*dydxi
       vol(indv) = MODIFIED_VOLUME(vol(indv))
       dxdeta = -vol(indv)*dydxi/g11
       dydeta = vol(indv)*dxdxi/g11
                
       b1 = dxdxi/g11
       b2 = dydxi/g11
       ba1 = b1*dxdeta
       ba2 = (b1*dydeta+b2*dxdeta)*0.5D0
        
       B(1,1,1) = ba1+beta2
       B(2,1,1) = ba2
       B(1,2,1) = ba2
       B(2,2,1) = -ba1+beta2
    
       C(1,1,1) = -ba1+beta2
       C(2,1,1) = -ba2
       C(1,2,1) = -ba2
       C(2,2,1) = ba1+beta2
    
       A(1,1,1) = 1.D0-2*beta2
       A(2,1,1) = 0.D0
       A(1,2,1) = 0.D0
       A(2,2,1) = 1.D0-2*beta2	

       indp2 = indice(4,j)
       indp1 = indice(3,j)
       ind = indice(2,j)
       indm1 = indice(1,j)
       indm2 = indice(ni-1,j)
                
       RHS(1,1)=-2.D0*vol(indv)*b2+xd(ind)+beta*(xd(indm2)- 
     &	   4.D0*xd(indm1)+ 6.D0*xd(ind)-4.D0*xd(indp1)+xd(indp2))
       RHS(2,1)= 2.D0*vol(indv)*b1+yd(ind)+beta*(yd(indm2)- 
     &	   4.D0*yd(indm1)+ 6.D0*yd(ind)-4.D0*yd(indp1)+yd(indp2))
    
C* i = ni-1
       indv = ni-1+(j-1)*ni
       indp1 = indice(ni,j)
       indm1 = indice(ni-2,j)
       dxdxi = (xd(indp1)-xd(indm1))*0.5D0
       dydxi = (yd(indp1)-yd(indm1))*0.5D0
       g11 = dxdxi*dxdxi+dydxi*dydxi
       vol(indv) = MODIFIED_VOLUME(vol(indv))
       dxdeta = -vol(indv)*dydxi/g11
       dydeta = vol(indv)*dxdxi/g11
    
       b1 = dxdxi/g11
       b2 = dydxi/g11
       ba1 = b1*dxdeta
       ba2 = (b1*dydeta+b2*dxdeta)*0.5D0
    
       B(1,1,ni-2) = ba1+beta2
       B(2,1,ni-2) = ba2
       B(1,2,ni-2) = ba2
       B(2,2,ni-2) = -ba1+beta2
       
       C(1,1,ni-2) = -ba1+beta2
       C(2,1,ni-2) = -ba2
       C(1,2,ni-2) = -ba2
       C(2,2,ni-2) = ba1+beta2
       
       A(1,1,ni-2) = 1.D0-2*beta2
       A(2,1,ni-2) = 0.D0
       A(1,2,ni-2) = 0.D0
       A(2,2,ni-2) = 1.D0-2*beta2

       indp2 = indice(1,j)
       indp1 = indice(ni,j)
       ind = indice(ni-1,j)
       indm1 = indice(ni-2,j)
       indm2 = indice(ni-3,j)

       RHS(1,ni-2)=-2.D0*vol(indv)*b2+xd(ind)+beta*(xd(indm2)- 
     &	   4.D0*xd(indm1)+ 6.D0*xd(ind)-4.D0*xd(indp1)+xd(indp2))
       RHS(2,ni-2)= 2.D0*vol(indv)*b1+yd(ind)+beta*(yd(indm2)- 
     &	   4.D0*yd(indm1)+ 6.D0*yd(ind)-4.D0*yd(indp1)+yd(indp2))   


C* i = ni	
       indv = ni+(j-1)*ni
       indp1 = indice(2,j)
       indm1 = indice(ni-1,j)
       dxdxi = (xd(indp1)-xd(indm1))*0.5D0
       dydxi = (yd(indp1)-yd(indm1))*0.5D0
       g11 = dxdxi*dxdxi+dydxi*dydxi
       vol(indv) = MODIFIED_VOLUME(vol(indv))
       dxdeta = -vol(indv)*dydxi/g11
       dydeta = vol(indv)*dxdxi/g11
    
       b1 = dxdxi/g11
       b2 = dydxi/g11
       ba1 = b1*dxdeta
       ba2 = (b1*dydeta+b2*dxdeta)*0.5D0
    
       B(1,1,ni-1) = ba1+beta2
       B(2,1,ni-1) = ba2
       B(1,2,ni-1) = ba2
       B(2,2,ni-1) = -ba1+beta2
       
       C(1,1,ni-1) = -ba1+beta2
       C(2,1,ni-1) = -ba2
       C(1,2,ni-1) = -ba2
       C(2,2,ni-1) = ba1+beta2
       
       A(1,1,ni-1) = 1.D0-2*beta2
       A(2,1,ni-1) = 0.D0
       A(1,2,ni-1) = 0.D0
       A(2,2,ni-1) = 1.D0-2*beta2

       indp2 = indice(3,j)
       indp1 = indice(2,j)
       ind = indice(ni,j)
       indm1 = indice(ni-1,j)
       indm2 = indice(ni-2,j)
                
       RHS(1,ni-1)=-2.D0*vol(indv)*b2+xd(ind)+beta*(xd(indm2)- 
     &	   4.D0*xd(indm1)+ 6.D0*xd(ind)-4.D0*xd(indp1)+xd(indp2))
       RHS(2,ni-1)= 2.D0*vol(indv)*b1+yd(ind)+beta*(yd(indm2)- 
     &	   4.D0*yd(indm1)+ 6.D0*yd(ind)-4.D0*yd(indp1)+yd(indp2))   
        
C* Inversion
C        DO i = 1, ni-1
C            WRITE(*,*) 'A',A(1,1,i),A(1,2,i)
C            WRITE(*,*) 'A',A(2,1,i),A(2,2,i)
C            WRITE(*,*) 'B',B(1,1,i),B(1,2,i)
C            WRITE(*,*) 'B',B(2,1,i),B(2,2,i)
C            WRITE(*,*) 'C',C(1,1,i),C(1,2,i)
C            WRITE(*,*) 'C',C(2,1,i),C(2,2,i)
C            WRITE(*,*) 'RHS',RHS(1,i),RHS(2,i)            
C        ENDDO

       CALL k6PTRID(deux,ni-1,C,A,B,RHS,Z,ZA,IP)
    
       DO i = 2, ni
          ind = indice(i,j+1)
          xd(ind) = RHS(1,i-1)
          yd(ind) = RHS(2,i-1)
       ENDDO
    
       ind = indice(1,j+1)
       indb = indice(ni,j+1)
       xd(ind) = xd(indb)
       yd(ind) = yd(indb)
    
      ENDIF
        
      ENDDO

      END
