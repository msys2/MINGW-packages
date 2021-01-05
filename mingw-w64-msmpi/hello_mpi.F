      program main

c*********************************************************************72
c
cc MAIN is the main program for HELLO_MPI.
c
c  Discussion:
c
c    HELLO is a simple MPI test program.
c
c    Each process prints out a "Hello, world!" message.
c
c    The master process also prints out a short message.
c
c  Licensing:
c
c    This code is distributed under the GNU LGPL license. 
c
c  Modified:
c
c    30 October 2008
c
c  Author:
c
c    John Burkardt
c
c  Reference:
c
c    William Gropp, Ewing Lusk, Anthony Skjellum,
c    Using MPI: Portable Parallel Programming with the
c    Message-Passing Interface,
c    Second Edition,
c    MIT Press, 1999,
c    ISBN: 0262571323,
c    LC: QA76.642.G76.
c

c
c  Define MPI symbols:
c
      include 'mpif.h'
c
c implicit none
c
      integer error
      integer id
      integer p
      double precision wtime
c
c  Initialize MPI.
c
      call MPI_Init ( error )
c
c  Get the number of processes.
c
      call MPI_Comm_size ( MPI_COMM_WORLD, p, error )
c
c  Get the individual process ID.
c
      call MPI_Comm_rank ( MPI_COMM_WORLD, id, error )
c
c  Print a message.
c
      if ( id .eq. 0 ) then
        call timestamp ( )
        write ( *, '(a)' ) ' '
        write ( *, '(a)' ) 'HELLO_MPI - Master process:'
        write ( *, '(a)' ) '  FORTRAN77/MPI version'
        write ( *, '(a)' ) '  A simple MPI program.'
        write ( *, '(a)' ) ' '
        write ( *, '(a,i3)' ) '  The number of processes is ', p
        write ( *, '(a)' ) ' '
      end if

      if ( id .eq. 0 ) then
        wtime = MPI_Wtime ( )
      end if

      write ( *, '(a,i3,a)' ) '  Process ', id, 
     &  ' says "Hello, world!"'

      if ( id .eq. 0 ) then
        wtime = MPI_Wtime ( ) - wtime
        write ( *, '(a)' ) ' '
        write ( *, '(a,g14.6,a)' ) 
     &  'Elapsed wall clock time = ', wtime, ' seconds.'

      end if
c
c  Terminate MPI.
c
      call MPI_Finalize ( error )
c
c  Terminate.
c
      if ( id .eq. 0 ) then
        write ( *, '(a)' ) ' '
        write ( *, '(a)' ) 'HELLO_MPI:'
        write ( *, '(a)' ) '  Normal end of execution.'
        write ( *, '(a)' ) ' '
        call timestamp ( )
      end if

      stop
      end
      subroutine timestamp ( )

c*********************************************************************72
c
cc TIMESTAMP prints out the current YMDHMS date as a timestamp.
c
c  Licensing:
c
c    This code is distributed under the GNU LGPL license.
c
c  Modified:
c
c    12 January 2007
c
c  Author:
c
c    John Burkardt
c
c  Parameters:
c
c    None
c
      implicit none

      character * ( 8 ) ampm
      integer d
      character * ( 8 ) date
      integer h
      integer m
      integer mm
      character * ( 9 ) month(12)
      integer n
      integer s
      character * ( 10 ) time
      integer y

      save month

      data month /
     &  'January  ', 'February ', 'March    ', 'April    ', 
     &  'May      ', 'June     ', 'July     ', 'August   ', 
     &  'September', 'October  ', 'November ', 'December ' /

      call date_and_time ( date, time )

      read ( date, '(i4,i2,i2)' ) y, m, d
      read ( time, '(i2,i2,i2,1x,i3)' ) h, n, s, mm

      if ( h .lt. 12 ) then
        ampm = 'AM'
      else if ( h .eq. 12 ) then
        if ( n .eq. 0 .and. s .eq. 0 ) then
          ampm = 'Noon'
        else
          ampm = 'PM'
        end if
      else
        h = h - 12
        if ( h .lt. 12 ) then
          ampm = 'PM'
        else if ( h .eq. 12 ) then
          if ( n .eq. 0 .and. s .eq. 0 ) then
            ampm = 'Midnight'
          else
            ampm = 'AM'
          end if
        end if
      end if

      write ( *, 
     &  '(i2,1x,a,1x,i4,2x,i2,a1,i2.2,a1,i2.2,a1,i3.3,1x,a)' ) 
     &  d, month(m), y, h, ':', n, ':', s, '.', mm, ampm

      return
      end
