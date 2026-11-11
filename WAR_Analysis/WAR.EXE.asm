;##############################################################################
; WAR.EXE  -  The Ancient Art of War (Broderbund, 1984)  -  ANNOTATED DISASSEMBLY
; Author (obfuscated credit @file 0x17842, +1 cipher): "COPYRIGHT 1983 DAVE MURRY"
; Linked with Phoenix Software Associates LINK86 overlay linker (C) 1983.
; Disassembly is a STRUCTURAL MAP + HOTSPOT annotation, not every byte.
; Addresses shown are IMAGE offsets (= file offset - 0x200).
;##############################################################################
; MZ HEADER
;   file size         = 98304 (0x18000)
;   header paragraphs = 32  -> header = 0x200 bytes
;   pages/last-page   = 26 / 240  -> declared image = 0x32F0 bytes
;   *** 85264 (0x14D10) bytes are APPENDED beyond declared image = OVERLAY AREA ***
;   relocations       = 71  (in frames 0x0000, 0x028D, 0x02F6)
;   initial SS:SP     = 1381:0100
;   entry CS:IP       = 028D:02FA  (image 0x2BCA, file 0x2DCA)
;
; MEMORY / FILE LAYOUT
;   file 0x00000-0x00200  MZ header + 71 relocation entries
;   file 0x00200-0x02AD0  C RUNTIME LIBRARY      (frame 0x0000, image 0x0000-0x28D0)
;   file 0x02AD0-0x030D0  ROOT: startup, overlay loader, root code (frame 0x028D)
;   file 0x030D0-0x03160  loader message strings ("$LINK86 Overlay Loader", etc.)
;   file 0x03160-0x031EE  OVERLAY DESCRIPTOR TABLE (per-overlay file pos & size)
;   file 0x031E6-0x031EE  "WAR.EXE" (name the loader re-opens to page overlays)
;   file 0x031EE-0x032F0  OVERLAY CALL THUNK TABLE (21 entry-point thunks)
;   file 0x032F0-0x18000  OVERLAY AREA (8 regions, paged in from WAR.EXE):
;       overlay 1 : file 0x032F0-0x08720  (21552 bytes)  game main loop + graphics/video (INT 10h) + core logic
;       overlay 2 : file 0x08720-0x08D00  ( 1504 bytes)  small module
;       overlay 3 : file 0x08D00-0x0A7F0  ( 6896 bytes)  module
;       overlay 4 : file 0x0A7F0-0x0B2C0  ( 2768 bytes)  module
;       overlay 5 : file 0x0B2C0-0x0C950  ( 5776 bytes)  module
;       overlay 6 : file 0x0C950-0x124C0  (23408 bytes)  large module
;       overlay 7 : file 0x124C0-0x172E0  (20000 bytes)  large module
;       overlay 8 : file 0x172E0-0x18000  ( 3360 bytes)  TEXT/MESSAGE DATA (all UI strings; not code)
;
; OVERLAY LOADER MECHANISM
;   A call to an overlaid routine goes to a 12-byte thunk in the table above:
;       9A d0 02 8d 02   lcall 028D:02D0     ; -> overlay_loader_vector
;       dw  <overlay#>                       ; which overlay to ensure resident
;       EA off 00 00     jmp far <off>:<seg> ; seg patched by loader after load
;   overlay_loader_vector pulls the overlay# from the return stream, then does
;   lcall 028D:0287 (overlay_loader_main) which, if that overlay is not resident,
;   opens WAR.EXE, LSEEKs to the overlay file position (descriptor table) and READs
;   it into the shared overlay segment, then transfers to the target routine.
;
; OVERLAY CALL THUNK TABLE (image offsets)
;   thunk  0 @02FEE  overlay 1  seg-off 0x30F0  -> ovl1_fn_30f0  (file 0x032F0)
;   thunk  1 @02FFA  overlay 1  seg-off 0x325F  -> ovl1_fn_325f  (file 0x0345F)
;   thunk  2 @03006  overlay 6  seg-off 0x30F0  -> ovl6_fn_30f0  (file 0x0C950)
;   thunk  3 @03012  overlay 7  seg-off 0x30F0  -> ovl7_fn_30f0  (file 0x124C0)
;   thunk  4 @0301E  overlay 1  seg-off 0x35C4  -> ovl1_fn_35c4  (file 0x037C4)
;   thunk  5 @0302A  overlay 1  seg-off 0x53AE  -> ovl1_fn_53ae  (file 0x055AE)
;   thunk  6 @03036  overlay 1  seg-off 0x3D1E  -> ovl1_fn_3d1e  (file 0x03F1E)
;   thunk  7 @03042  overlay 1  seg-off 0x4EB2  -> ovl1_fn_4eb2  (file 0x050B2)
;   thunk  8 @0304E  overlay 4  seg-off 0x30F0  -> ovl4_fn_30f0  (file 0x0A7F0)
;   thunk  9 @0305A  overlay 3  seg-off 0x30F0  -> ovl3_fn_30f0  (file 0x08D00)
;   thunk 10 @03066  overlay 1  seg-off 0x3E08  -> ovl1_fn_3e08  (file 0x04008)
;   thunk 11 @03072  overlay 1  seg-off 0x4000  -> ovl1_fn_4000  (file 0x04200)
;   thunk 12 @0307E  overlay 5  seg-off 0x30F0  -> ovl5_fn_30f0  (file 0x0B2C0)
;   thunk 13 @0308A  overlay 1  seg-off 0x5782  -> ovl1_fn_5782  (file 0x05982)
;   thunk 14 @03096  overlay 1  seg-off 0x39AA  -> ovl1_fn_39aa  (file 0x03BAA)
;   thunk 15 @030A2  overlay 1  seg-off 0x389B  -> ovl1_fn_389b  (file 0x03A9B)
;   thunk 16 @030AE  overlay 1  seg-off 0x3EB6  -> ovl1_fn_3eb6  (file 0x040B6)
;   thunk 17 @030BA  overlay 1  seg-off 0x3FC3  -> ovl1_fn_3fc3  (file 0x041C3)
;   thunk 18 @030C6  overlay 1  seg-off 0x4808  -> ovl1_fn_4808  (file 0x04A08)
;   thunk 19 @030D2  overlay 1  seg-off 0x7EDA  -> ovl1_fn_7eda  (file 0x080DA)
;   thunk 20 @030DE  overlay 2  seg-off 0x8520  -> ovl2_fn_8520  (file 0x08720)
;
; INTERRUPT USAGE (real sites, in code regions)
;   INT 21h  DOS services  - CONFIRMED in C library: open(3D) read(3F) write(40)
;            create(3C) close(3E) lseek(42) delete(41) print$(09) exec(4B)
;            alloc/resize(48/4A) getver(30) set/get-vector(25/35) exit(4C).
;   INT 10h  BIOS video  - CONFIRMED: set-mode, set-cursor, palette/border(0B)
;            -> CGA-class 4-colour graphics. Sites in root + overlay 1.
;   INT 16h/13h : 0xCD16/0xCD13 byte patterns occur inside overlay bodies that
;            are not disassembled here; likely BIOS keyboard / data-disk sector
;            access but treat as UNVERIFIED until those bodies are decoded.
;
; DATA FILES USED BY THE GAME (siblings of WAR.EXE in WAR_Original/)
;   CNF / CNFET      configuration
;   TITLES/DTITLES   title screen data
;   M                map / master data (11808 b)
;   F, WARW          flags / setup
;   O0..O8           "order"/scenario opponent files
;   P0..PO           scenario / campaign level files
;   T0..TA           terrain tiles (all exactly 1460 b -> fixed-size records)
;   WARET.DAT/.EXE   editor / extended data
;   (Filenames are short & built at runtime; see data-disk logic in overlay 1.)
;##############################################################################

;==============================================================================
; C RUNTIME LIBRARY  (frame 0x0000, image 0x0000-0x28D0)
; Compiled C startup-support & library: DOS/console I/O wrappers (INT 21h),
; memory, string and math helpers. Densely packed; near branches are relative
; so they resolve correctly in this flat image-offset view.
;==============================================================================
  00000  55                 push   bp                      
  00001  8bec               mov    bp, sp                  
  00003  83ec04             sub    sp, 4                   
  00006  33c0               xor    ax, ax                  
  00008  8806a9a9           mov    byte ptr [0xa9a9], al   
  0000c  817e040100         cmp    word ptr [bp + 4], 1    
  00011  7e27               jle    0x3a                    
  00013  8b7606             mov    si, word ptr [bp + 6]   
  00016  83c602             add    si, 2                   
  00019  8b34               mov    si, word ptr [si]       
  0001b  8a04               mov    al, byte ptr [si]       
  0001d  98                 cwde                           
  0001e  83f868             cmp    ax, 0x68                
  00021  7410               je     0x33                    
  00023  8b7606             mov    si, word ptr [bp + 6]   
  00026  83c602             add    si, 2                   
  00029  8b34               mov    si, word ptr [si]       
  0002b  8a04               mov    al, byte ptr [si]       
  0002d  98                 cwde                           
  0002e  83f848             cmp    ax, 0x48                
  00031  7507               jne    0x3a                    
  00033  b80100             mov    ax, 1                   
  00036  8806a9a9           mov    byte ptr [0xa9a9], al   
  0003a  e8b12f             call   0x2fee                  
  0003d  0bc0               or     ax, ax                  
  0003f  7403               je     0x44                    
  00041  e9d000             jmp    0x114                   
  00044  b80100             mov    ax, 1                   
  00047  0bc0               or     ax, ax                  
  00049  7503               jne    0x4e                    
  0004b  e9c600             jmp    0x114                   
  0004e  e8cd00             call   0x11e                   
  00051  0bc0               or     ax, ax                  
  00053  7503               jne    0x58                    
  00055  e9bc00             jmp    0x114                   
  00058  e89f2f             call   0x2ffa                  
  0005b  8946fd             mov    word ptr [bp - 3], ax   
  0005e  817efd0200         cmp    word ptr [bp - 3], 2    
  00063  7508               jne    0x6d                    
  00065  e83f01             call   0x1a7                   
  00068  8946fd             mov    word ptr [bp - 3], ax   
  0006b  ebd7               jmp    0x44                    
  0006d  817efd0600         cmp    word ptr [bp - 3], 6    
  00072  74d0               je     0x44                    
  00074  8b46fd             mov    ax, word ptr [bp - 3]   
  00077  0bc0               or     ax, ax                  
  00079  7403               je     0x7e                    
  0007b  e99600             jmp    0x114                   
  0007e  33c0               xor    ax, ax                  
  00080  890602a5           mov    word ptr [0xa502], ax   
  00084  89062f05           mov    word ptr [0x52f], ax    
  00088  b80100             mov    ax, 1                   
  0008b  0bc0               or     ax, ax                  
  0008d  7478               je     0x107                   
  0008f  e88c00             call   0x11e                   
  00092  0bc0               or     ax, ax                  
  00094  7502               jne    0x98                    
  00096  eb7c               jmp    0x114                   
  00098  e86b2f             call   0x3006                  
  0009b  8946fd             mov    word ptr [bp - 3], ax   
  0009e  83f802             cmp    ax, 2                   
  000a1  752a               jne    0xcd                    
  000a3  e87800             call   0x11e                   
  000a6  09c0               or     ax, ax                  
  000a8  746a               je     0x114                   
  000aa  e8652f             call   0x3012                  
  000ad  8946fd             mov    word ptr [bp - 3], ax   
  000b0  3d0f27             cmp    ax, 0x270f              
  000b3  745f               je     0x114                   
  000b5  817efd0100         cmp    word ptr [bp - 3], 1    
  000ba  744b               je     0x107                   
  000bc  8b06d8a6           mov    ax, word ptr [0xa6d8]   
  000c0  0bc0               or     ax, ax                  
  000c2  758a               jne    0x4e                    
  000c4  e85700             call   0x11e                   
  000c7  09c0               or     ax, ax                  
  000c9  7449               je     0x114                   
  000cb  eb28               jmp    0xf5                    
  000cd  817efd0f27         cmp    word ptr [bp - 3], 0x270f
  000d2  7440               je     0x114                   
  000d4  817efd0100         cmp    word ptr [bp - 3], 1    
  000d9  742c               je     0x107                   
  000db  817efd6400         cmp    word ptr [bp - 3], 0x64 
  000e0  7513               jne    0xf5                    
  000e2  e83900             call   0x11e                   
  000e5  09c0               or     ax, ax                  
  000e7  742b               je     0x114                   
  000e9  e8322f             call   0x301e                  
  000ec  0bc0               or     ax, ax                  
  000ee  7524               jne    0x114                   
  000f0  e83022             call   0x2323                  
  000f3  eb93               jmp    0x88                    
  000f5  8b06509e           mov    ax, word ptr [0x9e50]   
  000f9  89066a88           mov    word ptr [0x886a], ax   
  000fd  8b06529e           mov    ax, word ptr [0x9e52]   
  00101  89066888           mov    word ptr [0x8868], ax   
  00105  eb81               jmp    0x88                    
  00107  e81400             call   0x11e                   
  0010a  09c0               or     ax, ax                  
  0010c  7406               je     0x114                   
  0010e  e8192f             call   0x302a                  
  00111  e930ff             jmp    0x44                    
  00114  e83822             call   0x234f                  
  00117  e86821             call   0x2282                  
  0011a  8be5               mov    sp, bp                  
  0011c  5d                 pop    bp                      
  0011d  c3                 ret                            
  0011e  55                 push   bp                      
  0011f  8bec               mov    bp, sp                  
  00121  b8ffff             mov    ax, 0xffff              
  00124  50                 push   ax                      
  00125  e82000             call   0x148                   
  00128  5f                 pop    di                      
  00129  0bc0               or     ax, ax                  
  0012b  7514               jne    0x141                   
  0012d  33c0               xor    ax, ax                  
  0012f  50                 push   ax                      
  00130  e8cd02             call   0x400                   
  00133  5f                 pop    di                      
  00134  83f801             cmp    ax, 1                   
  00137  7506               jne    0x13f                   
  00139  33c0               xor    ax, ax                  
  0013b  8be5               mov    sp, bp                  
  0013d  5d                 pop    bp                      
  0013e  c3                 ret                            
  0013f  ebe0               jmp    0x121                   
  00141  b80100             mov    ax, 1                   
  00144  8be5               mov    sp, bp                  
  00146  5d                 pop    bp                      
  00147  c3                 ret                            
  00148  55                 push   bp                      
  00149  8bec               mov    bp, sp                  
  0014b  83ec02             sub    sp, 2                   
  0014e  817e040100         cmp    word ptr [bp + 4], 1    
  00153  7510               jne    0x165                   
  00155  813edca60200       cmp    word ptr [0xa6dc], 2    
  0015b  7408               je     0x165                   
  0015d  b81000             mov    ax, 0x10                
  00160  8946fe             mov    word ptr [bp - 2], ax   
  00163  eb15               jmp    0x17a                   
  00165  817e040000         cmp    word ptr [bp + 4], 0    
  0016a  7508               jne    0x174                   
  0016c  b81400             mov    ax, 0x14                
  0016f  8946fe             mov    word ptr [bp - 2], ax   
  00172  eb06               jmp    0x17a                   
  00174  b81800             mov    ax, 0x18                
  00177  8946fe             mov    word ptr [bp - 2], ax   
  0017a  b80400             mov    ax, 4                   
  0017d  50                 push   ax                      
  0017e  ff76fe             push   word ptr [bp - 2]       
  00181  e88624             call   0x260a                  
  00184  5f                 pop    di                      
  00185  5f                 pop    di                      
  00186  8906aaa9           mov    word ptr [0xa9aa], ax   
  0018a  813eaaa90000       cmp    word ptr [0xa9aa], 0    
  00190  7506               jne    0x198                   
  00192  33c0               xor    ax, ax                  
  00194  8be5               mov    sp, bp                  
  00196  5d                 pop    bp                      
  00197  c3                 ret                            
  00198  ff36aaa9           push   word ptr [0xa9aa]       
  0019c  e8eb24             call   0x268a                  
  0019f  5f                 pop    di                      
  001a0  b80100             mov    ax, 1                   
  001a3  8be5               mov    sp, bp                  
  001a5  5d                 pop    bp                      
  001a6  c3                 ret                            
  001a7  55                 push   bp                      
  001a8  8bec               mov    bp, sp                  
  001aa  83ec02             sub    sp, 2                   
  001ad  e8862e             call   0x3036                  
  001b0  89067ea6           mov    word ptr [0xa67e], ax   
  001b4  3d0f27             cmp    ax, 0x270f              
  001b7  7507               jne    0x1c0                   
  001b9  b80f27             mov    ax, 0x270f              
  001bc  8be5               mov    sp, bp                  
  001be  5d                 pop    bp                      
  001bf  c3                 ret                            
  001c0  813e7ea60000       cmp    word ptr [0xa67e], 0    
  001c6  7403               je     0x1cb                   
  001c8  e98000             jmp    0x24b                   
  001cb  e8742e             call   0x3042                  
  001ce  8946fe             mov    word ptr [bp - 2], ax   
  001d1  83f805             cmp    ax, 5                   
  001d4  7506               jne    0x1dc                   
  001d6  33c0               xor    ax, ax                  
  001d8  8be5               mov    sp, bp                  
  001da  5d                 pop    bp                      
  001db  c3                 ret                            
  001dc  8b46fe             mov    ax, word ptr [bp - 2]   
  001df  0bc0               or     ax, ax                  
  001e1  7407               je     0x1ea                   
  001e3  b80f27             mov    ax, 0x270f              
  001e6  8be5               mov    sp, bp                  
  001e8  5d                 pop    bp                      
  001e9  c3                 ret                            
  001ea  e831ff             call   0x11e                   
  001ed  09c0               or     ax, ax                  
  001ef  7507               jne    0x1f8                   
  001f1  b80f27             mov    ax, 0x270f              
  001f4  8be5               mov    sp, bp                  
  001f6  5d                 pop    bp                      
  001f7  c3                 ret                            
  001f8  b80300             mov    ax, 3                   
  001fb  890692a6           mov    word ptr [0xa692], ax   
  001ff  e84c2e             call   0x304e                  
  00202  8946fe             mov    word ptr [bp - 2], ax   
  00205  0bc0               or     ax, ax                  
  00207  7407               je     0x210                   
  00209  8b46fe             mov    ax, word ptr [bp - 2]   
  0020c  8be5               mov    sp, bp                  
  0020e  5d                 pop    bp                      
  0020f  c3                 ret                            
  00210  e80bff             call   0x11e                   
  00213  09c0               or     ax, ax                  
  00215  7507               jne    0x21e                   
  00217  b80f27             mov    ax, 0x270f              
  0021a  8be5               mov    sp, bp                  
  0021c  5d                 pop    bp                      
  0021d  c3                 ret                            
  0021e  b80900             mov    ax, 9                   
  00221  890692a6           mov    word ptr [0xa692], ax   
  00225  e8322e             call   0x305a                  
  00228  8946fe             mov    word ptr [bp - 2], ax   
  0022b  0bc0               or     ax, ax                  
  0022d  7407               je     0x236                   
  0022f  8b46fe             mov    ax, word ptr [bp - 2]   
  00232  8be5               mov    sp, bp                  
  00234  5d                 pop    bp                      
  00235  c3                 ret                            
  00236  e8e5fe             call   0x11e                   
  00239  09c0               or     ax, ax                  
  0023b  7507               jne    0x244                   
  0023d  b80f27             mov    ax, 0x270f              
  00240  8be5               mov    sp, bp                  
  00242  5d                 pop    bp                      
  00243  c3                 ret                            
  00244  e81f2e             call   0x3066                  
  00247  8be5               mov    sp, bp                  
  00249  5d                 pop    bp                      
  0024a  c3                 ret                            
  0024b  813e7ea60100       cmp    word ptr [0xa67e], 1    
  00251  7513               jne    0x266                   
  00253  e86300             call   0x2b9                   
  00256  8946fe             mov    word ptr [bp - 2], ax   
  00259  0bc0               or     ax, ax                  
  0025b  7407               je     0x264                   
  0025d  8b46fe             mov    ax, word ptr [bp - 2]   
  00260  8be5               mov    sp, bp                  
  00262  5d                 pop    bp                      
  00263  c3                 ret                            
  00264  eb4d               jmp    0x2b3                   
  00266  813e7ea60200       cmp    word ptr [0xa67e], 2    
  0026c  7507               jne    0x275                   
  0026e  e8012e             call   0x3072                  
  00271  8be5               mov    sp, bp                  
  00273  5d                 pop    bp                      
  00274  c3                 ret                            
  00275  813e7ea60300       cmp    word ptr [0xa67e], 3    
  0027b  7506               jne    0x283                   
  0027d  33c0               xor    ax, ax                  
  0027f  8be5               mov    sp, bp                  
  00281  5d                 pop    bp                      
  00282  c3                 ret                            
  00283  813e7ea60400       cmp    word ptr [0xa67e], 4    
  00289  7521               jne    0x2ac                   
  0028b  e890fe             call   0x11e                   
  0028e  09c0               or     ax, ax                  
  00290  7507               jne    0x299                   
  00292  b80f27             mov    ax, 0x270f              
  00295  8be5               mov    sp, bp                  
  00297  5d                 pop    bp                      
  00298  c3                 ret                            
  00299  e8e22d             call   0x307e                  
  0029c  e87ffe             call   0x11e                   
  0029f  09c0               or     ax, ax                  
  002a1  7507               jne    0x2aa                   
  002a3  b80f27             mov    ax, 0x270f              
  002a6  8be5               mov    sp, bp                  
  002a8  5d                 pop    bp                      
  002a9  c3                 ret                            
  002aa  eb07               jmp    0x2b3                   
  002ac  e8db2d             call   0x308a                  
  002af  8be5               mov    sp, bp                  
  002b1  5d                 pop    bp                      
  002b2  c3                 ret                            
  002b3  33c0               xor    ax, ax                  
  002b5  8be5               mov    sp, bp                  
  002b7  5d                 pop    bp                      
  002b8  c3                 ret                            
  002b9  55                 push   bp                      
  002ba  8bec               mov    bp, sp                  
  002bc  83ec04             sub    sp, 4                   
  002bf  8b06dca6           mov    ax, word ptr [0xa6dc]   
  002c3  8946fe             mov    word ptr [bp - 2], ax   
  002c6  33c0               xor    ax, ax                  
  002c8  8906dca6           mov    word ptr [0xa6dc], ax   
  002cc  b007               mov    al, 7                   
  002ce  890692a6           mov    word ptr [0xa692], ax   
  002d2  e8c12d             call   0x3096                  
  002d5  8946fc             mov    word ptr [bp - 4], ax   
  002d8  0bc0               or     ax, ax                  
  002da  740a               je     0x2e6                   
  002dc  e8c32d             call   0x30a2                  
  002df  8b46fc             mov    ax, word ptr [bp - 4]   
  002e2  8be5               mov    sp, bp                  
  002e4  5d                 pop    bp                      
  002e5  c3                 ret                            
  002e6  b80100             mov    ax, 1                   
  002e9  0bc0               or     ax, ax                  
  002eb  7503               jne    0x2f0                   
  002ed  e90301             jmp    0x3f3                   
  002f0  e8bb2d             call   0x30ae                  
  002f3  890680a6           mov    word ptr [0xa680], ax   
  002f7  3d0f27             cmp    ax, 0x270f              
  002fa  7507               jne    0x303                   
  002fc  b80f27             mov    ax, 0x270f              
  002ff  8be5               mov    sp, bp                  
  00301  5d                 pop    bp                      
  00302  c3                 ret                            
  00303  813e80a60000       cmp    word ptr [0xa680], 0    
  00309  751b               jne    0x326                   
  0030b  b80600             mov    ax, 6                   
  0030e  890692a6           mov    word ptr [0xa692], ax   
  00312  e8812d             call   0x3096                  
  00315  8946fc             mov    word ptr [bp - 4], ax   
  00318  0bc0               or     ax, ax                  
  0031a  7407               je     0x323                   
  0031c  8b46fc             mov    ax, word ptr [bp - 4]   
  0031f  8be5               mov    sp, bp                  
  00321  5d                 pop    bp                      
  00322  c3                 ret                            
  00323  e9ca00             jmp    0x3f0                   
  00326  813e80a60100       cmp    word ptr [0xa680], 1    
  0032c  751b               jne    0x349                   
  0032e  b80a00             mov    ax, 0xa                 
  00331  890692a6           mov    word ptr [0xa692], ax   
  00335  e85e2d             call   0x3096                  
  00338  8946fc             mov    word ptr [bp - 4], ax   
  0033b  0bc0               or     ax, ax                  
  0033d  7407               je     0x346                   
  0033f  8b46fc             mov    ax, word ptr [bp - 4]   
  00342  8be5               mov    sp, bp                  
  00344  5d                 pop    bp                      
  00345  c3                 ret                            
  00346  e9a700             jmp    0x3f0                   
  00349  813e80a60200       cmp    word ptr [0xa680], 2    
  0034f  753d               jne    0x38e                   
  00351  e8cafd             call   0x11e                   
  00354  09c0               or     ax, ax                  
  00356  7507               jne    0x35f                   
  00358  b80f27             mov    ax, 0x270f              
  0035b  8be5               mov    sp, bp                  
  0035d  5d                 pop    bp                      
  0035e  c3                 ret                            
  0035f  33c0               xor    ax, ax                  
  00361  890692a6           mov    word ptr [0xa692], ax   
  00365  e8e62c             call   0x304e                  
  00368  8946fc             mov    word ptr [bp - 4], ax   
  0036b  3d2823             cmp    ax, 0x2328              
  0036e  7e07               jle    0x377                   
  00370  8b46fc             mov    ax, word ptr [bp - 4]   
  00373  8be5               mov    sp, bp                  
  00375  5d                 pop    bp                      
  00376  c3                 ret                            
  00377  e8a4fd             call   0x11e                   
  0037a  09c0               or     ax, ax                  
  0037c  7507               jne    0x385                   
  0037e  b80f27             mov    ax, 0x270f              
  00381  8be5               mov    sp, bp                  
  00383  5d                 pop    bp                      
  00384  c3                 ret                            
  00385  ff76fc             push   word ptr [bp - 4]       
  00388  e82f2d             call   0x30ba                  
  0038b  5f                 pop    di                      
  0038c  eb62               jmp    0x3f0                   
  0038e  813e80a60300       cmp    word ptr [0xa680], 3    
  00394  745d               je     0x3f3                   
  00396  813e80a60400       cmp    word ptr [0xa680], 4    
  0039c  7517               jne    0x3b5                   
  0039e  b80a00             mov    ax, 0xa                 
  003a1  890692a6           mov    word ptr [0xa692], ax   
  003a5  e81e2d             call   0x30c6                  
  003a8  0bc0               or     ax, ax                  
  003aa  7407               je     0x3b3                   
  003ac  b80f27             mov    ax, 0x270f              
  003af  8be5               mov    sp, bp                  
  003b1  5d                 pop    bp                      
  003b2  c3                 ret                            
  003b3  eb3b               jmp    0x3f0                   
  003b5  e866fd             call   0x11e                   
  003b8  09c0               or     ax, ax                  
  003ba  7507               jne    0x3c3                   
  003bc  b80f27             mov    ax, 0x270f              
  003bf  8be5               mov    sp, bp                  
  003c1  5d                 pop    bp                      
  003c2  c3                 ret                            
  003c3  33c0               xor    ax, ax                  
  003c5  890692a6           mov    word ptr [0xa692], ax   
  003c9  e88e2c             call   0x305a                  
  003cc  8946fc             mov    word ptr [bp - 4], ax   
  003cf  3d2823             cmp    ax, 0x2328              
  003d2  7e07               jle    0x3db                   
  003d4  8b46fc             mov    ax, word ptr [bp - 4]   
  003d7  8be5               mov    sp, bp                  
  003d9  5d                 pop    bp                      
  003da  c3                 ret                            
  003db  e840fd             call   0x11e                   
  003de  09c0               or     ax, ax                  
  003e0  7507               jne    0x3e9                   
  003e2  b80f27             mov    ax, 0x270f              
  003e5  8be5               mov    sp, bp                  
  003e7  5d                 pop    bp                      
  003e8  c3                 ret                            
  003e9  ff76fc             push   word ptr [bp - 4]       
  003ec  e8cb2c             call   0x30ba                  
  003ef  5f                 pop    di                      
  003f0  e9f3fe             jmp    0x2e6                   
  003f3  8b46fe             mov    ax, word ptr [bp - 2]   
  003f6  8906dca6           mov    word ptr [0xa6dc], ax   
  003fa  33c0               xor    ax, ax                  
  003fc  8be5               mov    sp, bp                  
  003fe  5d                 pop    bp                      
  003ff  c3                 ret                            
  00400  55                 push   bp                      
  00401  8bec               mov    bp, sp                  
  00403  e8061f             call   0x230c                  
  00406  b81800             mov    ax, 0x18                
  00409  50                 push   ax                      
  0040a  e8d11a             call   0x1ede                  
  0040d  5f                 pop    di                      
  0040e  817e040100         cmp    word ptr [bp + 4], 1    
  00413  750a               jne    0x41f                   
  00415  b81a00             mov    ax, 0x1a                
  00418  50                 push   ax                      
  00419  e84921             call   0x2565                  
  0041c  5f                 pop    di                      
  0041d  eb08               jmp    0x427                   
  0041f  b87b05             mov    ax, 0x57b               
  00422  50                 push   ax                      
  00423  e83f21             call   0x2565                  
  00426  5f                 pop    di                      
  00427  b8a505             mov    ax, 0x5a5               
  0042a  50                 push   ax                      
  0042b  e83721             call   0x2565                  
  0042e  5f                 pop    di                      
  0042f  33c0               xor    ax, ax                  
  00431  89062f05           mov    word ptr [0x52f], ax    
  00435  b80100             mov    ax, 1                   
  00438  0bc0               or     ax, ax                  
  0043a  7428               je     0x464                   
  0043c  813e2f051300       cmp    word ptr [0x52f], 0x13  
  00442  750a               jne    0x44e                   
  00444  33c0               xor    ax, ax                  
  00446  89062f05           mov    word ptr [0x52f], ax    
  0044a  8be5               mov    sp, bp                  
  0044c  5d                 pop    bp                      
  0044d  c3                 ret                            
  0044e  813e2f051000       cmp    word ptr [0x52f], 0x10  
  00454  750c               jne    0x462                   
  00456  33c0               xor    ax, ax                  
  00458  89062f05           mov    word ptr [0x52f], ax    
  0045c  b001               mov    al, 1                   
  0045e  8be5               mov    sp, bp                  
  00460  5d                 pop    bp                      
  00461  c3                 ret                            
  00462  ebd1               jmp    0x435                   
  00464  8be5               mov    sp, bp                  
  00466  5d                 pop    bp                      
  00467  c3                 ret                            
  00468  55                 push   bp                      
  00469  8bec               mov    bp, sp                  
  0046b  83ec08             sub    sp, 8                   
  0046e  b80400             mov    ax, 4                   
  00471  50                 push   ax                      
  00472  b82400             mov    ax, 0x24                
  00475  50                 push   ax                      
  00476  e89121             call   0x260a                  
  00479  5f                 pop    di                      
  0047a  5f                 pop    di                      
  0047b  8906aaa9           mov    word ptr [0xa9aa], ax   
  0047f  813eaaa90000       cmp    word ptr [0xa9aa], 0    
  00485  7512               jne    0x499                   
  00487  33c0               xor    ax, ax                  
  00489  50                 push   ax                      
  0048a  e873ff             call   0x400                   
  0048d  5f                 pop    di                      
  0048e  09c0               or     ax, ax                  
  00490  74dc               je     0x46e                   
  00492  b80100             mov    ax, 1                   
  00495  8be5               mov    sp, bp                  
  00497  5d                 pop    bp                      
  00498  c3                 ret                            
  00499  b80100             mov    ax, 1                   
  0049c  8946f8             mov    word ptr [bp - 8], ax   
  0049f  817ef84900         cmp    word ptr [bp - 8], 0x49 
  004a4  7d72               jge    0x518                   
  004a6  b8680b             mov    ax, 0xb68               
  004a9  8946fe             mov    word ptr [bp - 2], ax   
  004ac  33c0               xor    ax, ax                  
  004ae  8946fa             mov    word ptr [bp - 6], ax   
  004b1  b0a4               mov    al, 0xa4                
  004b3  50                 push   ax                      
  004b4  b8d2a5             mov    ax, 0xa5d2              
  004b7  50                 push   ax                      
  004b8  ff36aaa9           push   word ptr [0xa9aa]       
  004bc  e88f21             call   0x264e                  
  004bf  83c406             add    sp, 6                   
  004c2  33c0               xor    ax, ax                  
  004c4  8946fc             mov    word ptr [bp - 4], ax   
  004c7  817efcc800         cmp    word ptr [bp - 4], 0xc8 
  004cc  7d45               jge    0x513                   
  004ce  be829d             mov    si, 0x9d82              
  004d1  0376fc             add    si, word ptr [bp - 4]   
  004d4  8a04               mov    al, byte ptr [si]       
  004d6  98                 cwde                           
  004d7  3b46f8             cmp    ax, word ptr [bp - 8]   
  004da  750c               jne    0x4e8                   
  004dc  ff76fe             push   word ptr [bp - 2]       
  004df  b8d2a5             mov    ax, 0xa5d2              
  004e2  50                 push   ax                      
  004e3  e84c17             call   0x1c32                  
  004e6  5f                 pop    di                      
  004e7  5f                 pop    di                      
  004e8  b80800             mov    ax, 8                   
  004eb  8b56fe             mov    dx, word ptr [bp - 2]   
  004ee  03d0               add    dx, ax                  
  004f0  8956fe             mov    word ptr [bp - 2], dx   
  004f3  ff46fa             inc    word ptr [bp - 6]       
  004f6  8b46fa             mov    ax, word ptr [bp - 6]   
  004f9  83f809             cmp    ax, 9                   
  004fc  7e10               jle    0x50e                   
  004fe  b8d002             mov    ax, 0x2d0               
  00501  8b56fe             mov    dx, word ptr [bp - 2]   
  00504  03d0               add    dx, ax                  
  00506  8956fe             mov    word ptr [bp - 2], dx   
  00509  33c0               xor    ax, ax                  
  0050b  8946fa             mov    word ptr [bp - 6], ax   
  0050e  ff46fc             inc    word ptr [bp - 4]       
  00511  ebb4               jmp    0x4c7                   
  00513  ff46f8             inc    word ptr [bp - 8]       
  00516  eb87               jmp    0x49f                   
  00518  ff36aaa9           push   word ptr [0xa9aa]       
  0051c  e86b21             call   0x268a                  
  0051f  5f                 pop    di                      
  00520  e8090a             call   0xf2c                   
  00523  33c0               xor    ax, ax                  
  00525  8be5               mov    sp, bp                  
  00527  5d                 pop    bp                      
  00528  c3                 ret                            
  00529  55                 push   bp                      
  0052a  8bec               mov    bp, sp                  
  0052c  8b064905           mov    ax, word ptr [0x549]    
  00530  0bc0               or     ax, ax                  
  00532  7423               je     0x557                   
  00534  8b064b05           mov    ax, word ptr [0x54b]    
  00538  0bc0               or     ax, ax                  
  0053a  741b               je     0x557                   
  0053c  8b064d05           mov    ax, word ptr [0x54d]    
  00540  0bc0               or     ax, ax                  
  00542  7413               je     0x557                   
  00544  e83b1d             call   0x2282                  
  00547  e8051e             call   0x234f                  
  0054a  e8852b             call   0x30d2                  
  0054d  e8321d             call   0x2282                  
  00550  33c0               xor    ax, ax                  
  00552  50                 push   ax                      
  00553  e88021             call   0x26d6                  
  00556  5f                 pop    di                      
  00557  8be5               mov    sp, bp                  
  00559  5d                 pop    bp                      
  0055a  c3                 ret                            
  0055b  55                 push   bp                      
  0055c  8bec               mov    bp, sp                  
  0055e  8b4604             mov    ax, word ptr [bp + 4]   
  00561  8b16c0a5           mov    dx, word ptr [0xa5c0]   
  00565  03d0               add    dx, ax                  
  00567  8916c0a5           mov    word ptr [0xa5c0], dx   
  0056b  83fa03             cmp    dx, 3                   
  0056e  7d06               jge    0x576                   
  00570  33c0               xor    ax, ax                  
  00572  8906c2a5           mov    word ptr [0xa5c2], ax   
  00576  b8c800             mov    ax, 0xc8                
  00579  50                 push   ax                      
  0057a  bb1000             mov    bx, 0x10                
  0057d  8b06c0a5           mov    ax, word ptr [0xa5c0]   
  00581  f7eb               imul   bx                      
  00583  5a                 pop    dx                      
  00584  2bd0               sub    dx, ax                  
  00586  83ea04             sub    dx, 4                   
  00589  8916bea5           mov    word ptr [0xa5be], dx   
  0058d  817e040000         cmp    word ptr [bp + 4], 0    
  00592  7d05               jge    0x599                   
  00594  33c0               xor    ax, ax                  
  00596  894604             mov    word ptr [bp + 4], ax   
  00599  8b4604             mov    ax, word ptr [bp + 4]   
  0059c  b90300             mov    cx, 3                   
  0059f  d3e0               shl    ax, cl                  
  005a1  50                 push   ax                      
  005a2  e84317             call   0x1ce8                  
  005a5  8be5               mov    sp, bp                  
  005a7  5d                 pop    bp                      
  005a8  c3                 ret                            
  005a9  55                 push   bp                      
  005aa  8bec               mov    bp, sp                  
  005ac  33c0               xor    ax, ax                  
  005ae  89063105           mov    word ptr [0x531], ax    
  005b2  e83a1b             call   0x20ef                  
  005b5  b8f401             mov    ax, 0x1f4               
  005b8  50                 push   ax                      
  005b9  e8201b             call   0x20dc                  
  005bc  5f                 pop    di                      
  005bd  b89001             mov    ax, 0x190               
  005c0  50                 push   ax                      
  005c1  e8c41b             call   0x2188                  
  005c4  5f                 pop    di                      
  005c5  e8351b             call   0x20fd                  
  005c8  8be5               mov    sp, bp                  
  005ca  5d                 pop    bp                      
  005cb  c3                 ret                            
  005cc  55                 push   bp                      
  005cd  8bec               mov    bp, sp                  
  005cf  e81d1b             call   0x20ef                  
  005d2  b8d007             mov    ax, 0x7d0               
  005d5  50                 push   ax                      
  005d6  e8031b             call   0x20dc                  
  005d9  5f                 pop    di                      
  005da  b82c01             mov    ax, 0x12c               
  005dd  50                 push   ax                      
  005de  e8a71b             call   0x2188                  
  005e1  5f                 pop    di                      
  005e2  e8181b             call   0x20fd                  
  005e5  8be5               mov    sp, bp                  
  005e7  5d                 pop    bp                      
  005e8  c3                 ret                            
  005e9  55                 push   bp                      
  005ea  8bec               mov    bp, sp                  
  005ec  e8001b             call   0x20ef                  
  005ef  b8401f             mov    ax, 0x1f40              
  005f2  50                 push   ax                      
  005f3  e8e61a             call   0x20dc                  
  005f6  5f                 pop    di                      
  005f7  b85802             mov    ax, 0x258               
  005fa  50                 push   ax                      
  005fb  e88a1b             call   0x2188                  
  005fe  5f                 pop    di                      
  005ff  e8fb1a             call   0x20fd                  
  00602  8be5               mov    sp, bp                  
  00604  5d                 pop    bp                      
  00605  c3                 ret                            
  00606  55                 push   bp                      
  00607  8bec               mov    bp, sp                  
  00609  813e41050000       cmp    word ptr [0x541], 0     
  0060f  750b               jne    0x61c                   
  00611  b80100             mov    ax, 1                   
  00614  50                 push   ax                      
  00615  e8b01c             call   0x22c8                  
  00618  8be5               mov    sp, bp                  
  0061a  5d                 pop    bp                      
  0061b  c3                 ret                            
  0061c  813edfa60200       cmp    word ptr [0xa6df], 2    
  00622  7531               jne    0x655                   
  00624  ff7614             push   word ptr [bp + 0x14]    
  00627  33c0               xor    ax, ax                  
  00629  50                 push   ax                      
  0062a  e8b51c             call   0x22e2                  
  0062d  5f                 pop    di                      
  0062e  5f                 pop    di                      
  0062f  ff7616             push   word ptr [bp + 0x16]    
  00632  b80100             mov    ax, 1                   
  00635  50                 push   ax                      
  00636  e8a91c             call   0x22e2                  
  00639  5f                 pop    di                      
  0063a  5f                 pop    di                      
  0063b  ff7618             push   word ptr [bp + 0x18]    
  0063e  b80200             mov    ax, 2                   
  00641  50                 push   ax                      
  00642  e89d1c             call   0x22e2                  
  00645  5f                 pop    di                      
  00646  5f                 pop    di                      
  00647  ff761a             push   word ptr [bp + 0x1a]    
  0064a  b80300             mov    ax, 3                   
  0064d  50                 push   ax                      
  0064e  e8911c             call   0x22e2                  
  00651  5f                 pop    di                      
  00652  5f                 pop    di                      
  00653  eb68               jmp    0x6bd                   
  00655  813edfa60100       cmp    word ptr [0xa6df], 1    
  0065b  7531               jne    0x68e                   
  0065d  ff7604             push   word ptr [bp + 4]       
  00660  33c0               xor    ax, ax                  
  00662  50                 push   ax                      
  00663  e87c1c             call   0x22e2                  
  00666  5f                 pop    di                      
  00667  5f                 pop    di                      
  00668  ff7606             push   word ptr [bp + 6]       
  0066b  b80100             mov    ax, 1                   
  0066e  50                 push   ax                      
  0066f  e8701c             call   0x22e2                  
  00672  5f                 pop    di                      
  00673  5f                 pop    di                      
  00674  ff7608             push   word ptr [bp + 8]       
  00677  b80200             mov    ax, 2                   
  0067a  50                 push   ax                      
  0067b  e8641c             call   0x22e2                  
  0067e  5f                 pop    di                      
  0067f  5f                 pop    di                      
  00680  ff760a             push   word ptr [bp + 0xa]     
  00683  b80300             mov    ax, 3                   
  00686  50                 push   ax                      
  00687  e8581c             call   0x22e2                  
  0068a  5f                 pop    di                      
  0068b  5f                 pop    di                      
  0068c  eb2f               jmp    0x6bd                   
  0068e  ff760c             push   word ptr [bp + 0xc]     
  00691  33c0               xor    ax, ax                  
  00693  50                 push   ax                      
  00694  e84b1c             call   0x22e2                  
  00697  5f                 pop    di                      
  00698  5f                 pop    di                      
  00699  ff760e             push   word ptr [bp + 0xe]     
  0069c  b80100             mov    ax, 1                   
  0069f  50                 push   ax                      
  006a0  e83f1c             call   0x22e2                  
  006a3  5f                 pop    di                      
  006a4  5f                 pop    di                      
  006a5  ff7610             push   word ptr [bp + 0x10]    
  006a8  b80200             mov    ax, 2                   
  006ab  50                 push   ax                      
  006ac  e8331c             call   0x22e2                  
  006af  5f                 pop    di                      
  006b0  5f                 pop    di                      
  006b1  ff7612             push   word ptr [bp + 0x12]    
  006b4  b80300             mov    ax, 3                   
  006b7  50                 push   ax                      
  006b8  e8271c             call   0x22e2                  
  006bb  5f                 pop    di                      
  006bc  5f                 pop    di                      
  006bd  8be5               mov    sp, bp                  
  006bf  5d                 pop    bp                      
  006c0  c3                 ret                            
  006c1  55                 push   bp                      
  006c2  8bec               mov    bp, sp                  
  006c4  b81800             mov    ax, 0x18                
  006c7  50                 push   ax                      
  006c8  e81318             call   0x1ede                  
  006cb  5f                 pop    di                      
  006cc  b81800             mov    ax, 0x18                
  006cf  50                 push   ax                      
  006d0  b00a               mov    al, 0xa                 
  006d2  50                 push   ax                      
  006d3  e88619             call   0x205c                  
  006d6  5f                 pop    di                      
  006d7  5f                 pop    di                      
  006d8  b82600             mov    ax, 0x26                
  006db  50                 push   ax                      
  006dc  e8861e             call   0x2565                  
  006df  8be5               mov    sp, bp                  
  006e1  5d                 pop    bp                      
  006e2  c3                 ret                            
  006e3  55                 push   bp                      
  006e4  8bec               mov    bp, sp                  
  006e6  83ec04             sub    sp, 4                   
  006e9  8b06d4a6           mov    ax, word ptr [0xa6d4]   
  006ed  40                 inc    ax                      
  006ee  8946fe             mov    word ptr [bp - 2], ax   
  006f1  817efe4f00         cmp    word ptr [bp - 2], 0x4f 
  006f6  7d22               jge    0x71a                   
  006f8  b81800             mov    ax, 0x18                
  006fb  50                 push   ax                      
  006fc  ff76fe             push   word ptr [bp - 2]       
  006ff  e88518             call   0x1f87                  
  00702  5f                 pop    di                      
  00703  5f                 pop    di                      
  00704  8946fc             mov    word ptr [bp - 4], ax   
  00707  817efc4100         cmp    word ptr [bp - 4], 0x41 
  0070c  7c07               jl     0x715                   
  0070e  817efc5a00         cmp    word ptr [bp - 4], 0x5a 
  00713  7e05               jle    0x71a                   
  00715  ff46fe             inc    word ptr [bp - 2]       
  00718  ebd7               jmp    0x6f1                   
  0071a  817efe4f00         cmp    word ptr [bp - 2], 0x4f 
  0071f  7d11               jge    0x732                   
  00721  8b46fe             mov    ax, word ptr [bp - 2]   
  00724  8906d4a6           mov    word ptr [0xa6d4], ax   
  00728  b8a00f             mov    ax, 0xfa0               
  0072b  50                 push   ax                      
  0072c  e8591a             call   0x2188                  
  0072f  5f                 pop    di                      
  00730  eb06               jmp    0x738                   
  00732  33c0               xor    ax, ax                  
  00734  8906d4a6           mov    word ptr [0xa6d4], ax   
  00738  8be5               mov    sp, bp                  
  0073a  5d                 pop    bp                      
  0073b  c3                 ret                            
  0073c  55                 push   bp                      
  0073d  8bec               mov    bp, sp                  
  0073f  83ec04             sub    sp, 4                   
  00742  8b06d4a6           mov    ax, word ptr [0xa6d4]   
  00746  48                 dec    ax                      
  00747  8946fe             mov    word ptr [bp - 2], ax   
  0074a  817efe0000         cmp    word ptr [bp - 2], 0    
  0074f  7c22               jl     0x773                   
  00751  b81800             mov    ax, 0x18                
  00754  50                 push   ax                      
  00755  ff76fe             push   word ptr [bp - 2]       
  00758  e82c18             call   0x1f87                  
  0075b  5f                 pop    di                      
  0075c  5f                 pop    di                      
  0075d  8946fc             mov    word ptr [bp - 4], ax   
  00760  817efc4100         cmp    word ptr [bp - 4], 0x41 
  00765  7c07               jl     0x76e                   
  00767  817efc5a00         cmp    word ptr [bp - 4], 0x5a 
  0076c  7e05               jle    0x773                   
  0076e  ff4efe             dec    word ptr [bp - 2]       
  00771  ebd7               jmp    0x74a                   
  00773  817efe0000         cmp    word ptr [bp - 2], 0    
  00778  7c11               jl     0x78b                   
  0077a  8b46fe             mov    ax, word ptr [bp - 2]   
  0077d  8906d4a6           mov    word ptr [0xa6d4], ax   
  00781  b8a00f             mov    ax, 0xfa0               
  00784  50                 push   ax                      
  00785  e8001a             call   0x2188                  
  00788  5f                 pop    di                      
  00789  eb1a               jmp    0x7a5                   
  0078b  813ed4a64e00       cmp    word ptr [0xa6d4], 0x4e 
  00791  740c               je     0x79f                   
  00793  b84e00             mov    ax, 0x4e                
  00796  8906d4a6           mov    word ptr [0xa6d4], ax   
  0079a  e89fff             call   0x73c                   
  0079d  eb06               jmp    0x7a5                   
  0079f  33c0               xor    ax, ax                  
  007a1  8906d4a6           mov    word ptr [0xa6d4], ax   
  007a5  8be5               mov    sp, bp                  
  007a7  5d                 pop    bp                      
  007a8  c3                 ret                            
  007a9  55                 push   bp                      
  007aa  8bec               mov    bp, sp                  
  007ac  83ec06             sub    sp, 6                   
  007af  b80100             mov    ax, 1                   
  007b2  8946fc             mov    word ptr [bp - 4], ax   
  007b5  8b06d4a6           mov    ax, word ptr [0xa6d4]   
  007b9  40                 inc    ax                      
  007ba  8946fa             mov    word ptr [bp - 6], ax   
  007bd  817efa4f00         cmp    word ptr [bp - 6], 0x4f 
  007c2  7d27               jge    0x7eb                   
  007c4  b81800             mov    ax, 0x18                
  007c7  50                 push   ax                      
  007c8  ff76fa             push   word ptr [bp - 6]       
  007cb  e8b917             call   0x1f87                  
  007ce  5f                 pop    di                      
  007cf  5f                 pop    di                      
  007d0  8946fe             mov    word ptr [bp - 2], ax   
  007d3  817efe6100         cmp    word ptr [bp - 2], 0x61 
  007d8  7c07               jl     0x7e1                   
  007da  817efe7a00         cmp    word ptr [bp - 2], 0x7a 
  007df  7e02               jle    0x7e3                   
  007e1  eb08               jmp    0x7eb                   
  007e3  ff46fc             inc    word ptr [bp - 4]       
  007e6  ff46fa             inc    word ptr [bp - 6]       
  007e9  ebd2               jmp    0x7bd                   
  007eb  8b46fc             mov    ax, word ptr [bp - 4]   
  007ee  8906d6a6           mov    word ptr [0xa6d6], ax   
  007f2  8be5               mov    sp, bp                  
  007f4  5d                 pop    bp                      
  007f5  c3                 ret                            
  007f6  55                 push   bp                      
  007f7  8bec               mov    bp, sp                  
  007f9  8b06d2a6           mov    ax, word ptr [0xa6d2]   
  007fd  0bc0               or     ax, ax                  
  007ff  7425               je     0x826                   
  00801  33c0               xor    ax, ax                  
  00803  8906d4a6           mov    word ptr [0xa6d4], ax   
  00807  b018               mov    al, 0x18                
  00809  50                 push   ax                      
  0080a  33c0               xor    ax, ax                  
  0080c  50                 push   ax                      
  0080d  e87717             call   0x1f87                  
  00810  5f                 pop    di                      
  00811  5f                 pop    di                      
  00812  09c0               or     ax, ax                  
  00814  7503               jne    0x819                   
  00816  e8cafe             call   0x6e3                   
  00819  e88dff             call   0x7a9                   
  0081c  50                 push   ax                      
  0081d  ff36d4a6           push   word ptr [0xa6d4]       
  00821  e8f317             call   0x2017                  
  00824  5f                 pop    di                      
  00825  5f                 pop    di                      
  00826  8be5               mov    sp, bp                  
  00828  5d                 pop    bp                      
  00829  c3                 ret                            
  0082a  55                 push   bp                      
  0082b  8bec               mov    bp, sp                  
  0082d  8b06d2a6           mov    ax, word ptr [0xa6d2]   
  00831  0bc0               or     ax, ax                  
  00833  7420               je     0x855                   
  00835  b81800             mov    ax, 0x18                
  00838  50                 push   ax                      
  00839  33c0               xor    ax, ax                  
  0083b  50                 push   ax                      
  0083c  e84817             call   0x1f87                  
  0083f  5f                 pop    di                      
  00840  5f                 pop    di                      
  00841  09c0               or     ax, ax                  
  00843  7503               jne    0x848                   
  00845  e8f4fe             call   0x73c                   
  00848  e85eff             call   0x7a9                   
  0084b  50                 push   ax                      
  0084c  ff36d4a6           push   word ptr [0xa6d4]       
  00850  e8c417             call   0x2017                  
  00853  5f                 pop    di                      
  00854  5f                 pop    di                      
  00855  8be5               mov    sp, bp                  
  00857  5d                 pop    bp                      
  00858  c3                 ret                            
  00859  55                 push   bp                      
  0085a  8bec               mov    bp, sp                  
  0085c  83ec02             sub    sp, 2                   
  0085f  be90a0             mov    si, 0xa090              
  00862  8b4604             mov    ax, word ptr [bp + 4]   
  00865  d1e0               shl    ax, 1                   
  00867  03f0               add    si, ax                  
  00869  8b04               mov    ax, word ptr [si]       
  0086b  83f802             cmp    ax, 2                   
  0086e  7518               jne    0x888                   
  00870  be90a0             mov    si, 0xa090              
  00873  8b4606             mov    ax, word ptr [bp + 6]   
  00876  d1e0               shl    ax, 1                   
  00878  03f0               add    si, ax                  
  0087a  8b04               mov    ax, word ptr [si]       
  0087c  83f802             cmp    ax, 2                   
  0087f  7507               jne    0x888                   
  00881  33c0               xor    ax, ax                  
  00883  8946fe             mov    word ptr [bp - 2], ax   
  00886  eb60               jmp    0x8e8                   
  00888  be90a0             mov    si, 0xa090              
  0088b  8b4604             mov    ax, word ptr [bp + 4]   
  0088e  d1e0               shl    ax, 1                   
  00890  03f0               add    si, ax                  
  00892  8b04               mov    ax, word ptr [si]       
  00894  83f802             cmp    ax, 2                   
  00897  7508               jne    0x8a1                   
  00899  b8bdff             mov    ax, 0xffbd              
  0089c  8946fe             mov    word ptr [bp - 2], ax   
  0089f  eb47               jmp    0x8e8                   
  008a1  be90a0             mov    si, 0xa090              
  008a4  8b4606             mov    ax, word ptr [bp + 6]   
  008a7  d1e0               shl    ax, 1                   
  008a9  03f0               add    si, ax                  
  008ab  8b04               mov    ax, word ptr [si]       
  008ad  83f802             cmp    ax, 2                   
  008b0  7508               jne    0x8ba                   
  008b2  b84300             mov    ax, 0x43                
  008b5  8946fe             mov    word ptr [bp - 2], ax   
  008b8  eb2e               jmp    0x8e8                   
  008ba  ff7606             push   word ptr [bp + 6]       
  008bd  ff7604             push   word ptr [bp + 4]       
  008c0  e82c00             call   0x8ef                   
  008c3  5f                 pop    di                      
  008c4  5f                 pop    di                      
  008c5  d1f8               sar    ax, 1                   
  008c7  50                 push   ax                      
  008c8  bef09f             mov    si, 0x9ff0              
  008cb  037604             add    si, word ptr [bp + 4]   
  008ce  8a04               mov    al, byte ptr [si]       
  008d0  98                 cwde                           
  008d1  50                 push   ax                      
  008d2  bef09f             mov    si, 0x9ff0              
  008d5  037606             add    si, word ptr [bp + 6]   
  008d8  8a04               mov    al, byte ptr [si]       
  008da  98                 cwde                           
  008db  5a                 pop    dx                      
  008dc  2bd0               sub    dx, ax                  
  008de  d1e2               shl    dx, 1                   
  008e0  d1e2               shl    dx, 1                   
  008e2  58                 pop    ax                      
  008e3  03c2               add    ax, dx                  
  008e5  8946fe             mov    word ptr [bp - 2], ax   
  008e8  8b46fe             mov    ax, word ptr [bp - 2]   
  008eb  8be5               mov    sp, bp                  
  008ed  5d                 pop    bp                      
  008ee  c3                 ret                            
  008ef  55                 push   bp                      
  008f0  8bec               mov    bp, sp                  
  008f2  83ec02             sub    sp, 2                   
  008f5  be3c9f             mov    si, 0x9f3c              
  008f8  037604             add    si, word ptr [bp + 4]   
  008fb  8a04               mov    al, byte ptr [si]       
  008fd  98                 cwde                           
  008fe  50                 push   ax                      
  008ff  be3c9f             mov    si, 0x9f3c              
  00902  037606             add    si, word ptr [bp + 6]   
  00905  8a04               mov    al, byte ptr [si]       
  00907  98                 cwde                           
  00908  5a                 pop    dx                      
  00909  2bd0               sub    dx, ax                  
  0090b  8956fe             mov    word ptr [bp - 2], dx   
  0090e  be829d             mov    si, 0x9d82              
  00911  fff6               push   si                      
  00913  ff7604             push   word ptr [bp + 4]       
  00916  e8170a             call   0x1330                  
  00919  5f                 pop    di                      
  0091a  5e                 pop    si                      
  0091b  03f0               add    si, ax                  
  0091d  8a04               mov    al, byte ptr [si]       
  0091f  98                 cwde                           
  00920  50                 push   ax                      
  00921  e8d408             call   0x11f8                  
  00924  5f                 pop    di                      
  00925  0bc0               or     ax, ax                  
  00927  7405               je     0x92e                   
  00929  8146fe0f00         add    word ptr [bp - 2], 0xf  
  0092e  be829d             mov    si, 0x9d82              
  00931  fff6               push   si                      
  00933  ff7606             push   word ptr [bp + 6]       
  00936  e8f709             call   0x1330                  
  00939  5f                 pop    di                      
  0093a  5e                 pop    si                      
  0093b  03f0               add    si, ax                  
  0093d  8a04               mov    al, byte ptr [si]       
  0093f  98                 cwde                           
  00940  50                 push   ax                      
  00941  e8b408             call   0x11f8                  
  00944  5f                 pop    di                      
  00945  0bc0               or     ax, ax                  
  00947  7405               je     0x94e                   
  00949  816efe0f00         sub    word ptr [bp - 2], 0xf  
  0094e  be829d             mov    si, 0x9d82              
  00951  fff6               push   si                      
  00953  ff7604             push   word ptr [bp + 4]       
  00956  e8d709             call   0x1330                  
  00959  5f                 pop    di                      
  0095a  5e                 pop    si                      
  0095b  03f0               add    si, ax                  
  0095d  8a04               mov    al, byte ptr [si]       
  0095f  98                 cwde                           
  00960  50                 push   ax                      
  00961  e8f408             call   0x1258                  
  00964  5f                 pop    di                      
  00965  0bc0               or     ax, ax                  
  00967  7511               jne    0x97a                   
  00969  be90a0             mov    si, 0xa090              
  0096c  8b4604             mov    ax, word ptr [bp + 4]   
  0096f  d1e0               shl    ax, 1                   
  00971  03f0               add    si, ax                  
  00973  8b04               mov    ax, word ptr [si]       
  00975  83f802             cmp    ax, 2                   
  00978  7506               jne    0x980                   
  0097a  b8e2ff             mov    ax, 0xffe2              
  0097d  8946fe             mov    word ptr [bp - 2], ax   
  00980  be829d             mov    si, 0x9d82              
  00983  fff6               push   si                      
  00985  ff7606             push   word ptr [bp + 6]       
  00988  e8a509             call   0x1330                  
  0098b  5f                 pop    di                      
  0098c  5e                 pop    si                      
  0098d  03f0               add    si, ax                  
  0098f  8a04               mov    al, byte ptr [si]       
  00991  98                 cwde                           
  00992  50                 push   ax                      
  00993  e8c208             call   0x1258                  
  00996  5f                 pop    di                      
  00997  0bc0               or     ax, ax                  
  00999  7511               jne    0x9ac                   
  0099b  be90a0             mov    si, 0xa090              
  0099e  8b4606             mov    ax, word ptr [bp + 6]   
  009a1  d1e0               shl    ax, 1                   
  009a3  03f0               add    si, ax                  
  009a5  8b04               mov    ax, word ptr [si]       
  009a7  83f802             cmp    ax, 2                   
  009aa  7506               jne    0x9b2                   
  009ac  b81e00             mov    ax, 0x1e                
  009af  8946fe             mov    word ptr [bp - 2], ax   
  009b2  817efe1e00         cmp    word ptr [bp - 2], 0x1e 
  009b7  7e06               jle    0x9bf                   
  009b9  b81e00             mov    ax, 0x1e                
  009bc  8946fe             mov    word ptr [bp - 2], ax   
  009bf  817efee2ff         cmp    word ptr [bp - 2], 0xffe2
  009c4  7d06               jge    0x9cc                   
  009c6  b8e2ff             mov    ax, 0xffe2              
  009c9  8946fe             mov    word ptr [bp - 2], ax   
  009cc  8b46fe             mov    ax, word ptr [bp - 2]   
  009cf  8be5               mov    sp, bp                  
  009d1  5d                 pop    bp                      
  009d2  c3                 ret                            
  009d3  55                 push   bp                      
  009d4  8bec               mov    bp, sp                  
  009d6  83ec0a             sub    sp, 0xa                 
  009d9  bea09f             mov    si, 0x9fa0              
  009dc  037606             add    si, word ptr [bp + 6]   
  009df  8a04               mov    al, byte ptr [si]       
  009e1  98                 cwde                           
  009e2  8946f8             mov    word ptr [bp - 8], ax   
  009e5  bec89f             mov    si, 0x9fc8              
  009e8  037606             add    si, word ptr [bp + 6]   
  009eb  8a04               mov    al, byte ptr [si]       
  009ed  98                 cwde                           
  009ee  8946fa             mov    word ptr [bp - 6], ax   
  009f1  beb49f             mov    si, 0x9fb4              
  009f4  037606             add    si, word ptr [bp + 6]   
  009f7  8a04               mov    al, byte ptr [si]       
  009f9  98                 cwde                           
  009fa  8946fc             mov    word ptr [bp - 4], ax   
  009fd  33c0               xor    ax, ax                  
  009ff  8946f6             mov    word ptr [bp - 0xa], ax 
  00a02  817ef60e00         cmp    word ptr [bp - 0xa], 0xe
  00a07  7c03               jl     0xa0c                   
  00a09  e91401             jmp    0xb20                   
  00a0c  b8329c             mov    ax, 0x9c32              
  00a0f  8b5604             mov    dx, word ptr [bp + 4]   
  00a12  d1e2               shl    dx, 1                   
  00a14  03c2               add    ax, dx                  
  00a16  8946fe             mov    word ptr [bp - 2], ax   
  00a19  8b36bba8           mov    si, word ptr [0xa8bb]   
  00a1d  0376f6             add    si, word ptr [bp - 0xa] 
  00a20  8a04               mov    al, byte ptr [si]       
  00a22  98                 cwde                           
  00a23  8b76fe             mov    si, word ptr [bp - 2]   
  00a26  8904               mov    word ptr [si], ax       
  00a28  817e080000         cmp    word ptr [bp + 8], 0    
  00a2d  7505               jne    0xa34                   
  00a2f  b8ffff             mov    ax, 0xffff              
  00a32  eb02               jmp    0xa36                   
  00a34  33c0               xor    ax, ax                  
  00a36  be6a9c             mov    si, 0x9c6a              
  00a39  8b5604             mov    dx, word ptr [bp + 4]   
  00a3c  d1e2               shl    dx, 1                   
  00a3e  03f2               add    si, dx                  
  00a40  8904               mov    word ptr [si], ax       
  00a42  8b76fe             mov    si, word ptr [bp - 2]   
  00a45  8b04               mov    ax, word ptr [si]       
  00a47  83f802             cmp    ax, 2                   
  00a4a  750c               jne    0xa58                   
  00a4c  8b46f8             mov    ax, word ptr [bp - 8]   
  00a4f  0bc0               or     ax, ax                  
  00a51  7405               je     0xa58                   
  00a53  ff4ef8             dec    word ptr [bp - 8]       
  00a56  eb68               jmp    0xac0                   
  00a58  8b76fe             mov    si, word ptr [bp - 2]   
  00a5b  8b04               mov    ax, word ptr [si]       
  00a5d  83f801             cmp    ax, 1                   
  00a60  750c               jne    0xa6e                   
  00a62  8b46fa             mov    ax, word ptr [bp - 6]   
  00a65  0bc0               or     ax, ax                  
  00a67  7405               je     0xa6e                   
  00a69  ff4efa             dec    word ptr [bp - 6]       
  00a6c  eb52               jmp    0xac0                   
  00a6e  8b76fe             mov    si, word ptr [bp - 2]   
  00a71  8b04               mov    ax, word ptr [si]       
  00a73  09c0               or     ax, ax                  
  00a75  750c               jne    0xa83                   
  00a77  8b46fc             mov    ax, word ptr [bp - 4]   
  00a7a  0bc0               or     ax, ax                  
  00a7c  7405               je     0xa83                   
  00a7e  ff4efc             dec    word ptr [bp - 4]       
  00a81  eb3d               jmp    0xac0                   
  00a83  8b46fc             mov    ax, word ptr [bp - 4]   
  00a86  0bc0               or     ax, ax                  
  00a88  740c               je     0xa96                   
  00a8a  ff4efc             dec    word ptr [bp - 4]       
  00a8d  33c0               xor    ax, ax                  
  00a8f  8b76fe             mov    si, word ptr [bp - 2]   
  00a92  8904               mov    word ptr [si], ax       
  00a94  eb2a               jmp    0xac0                   
  00a96  8b46fa             mov    ax, word ptr [bp - 6]   
  00a99  0bc0               or     ax, ax                  
  00a9b  740d               je     0xaaa                   
  00a9d  ff4efa             dec    word ptr [bp - 6]       
  00aa0  b80100             mov    ax, 1                   
  00aa3  8b76fe             mov    si, word ptr [bp - 2]   
  00aa6  8904               mov    word ptr [si], ax       
  00aa8  eb16               jmp    0xac0                   
  00aaa  8b46f8             mov    ax, word ptr [bp - 8]   
  00aad  0bc0               or     ax, ax                  
  00aaf  740d               je     0xabe                   
  00ab1  ff4ef8             dec    word ptr [bp - 8]       
  00ab4  b80200             mov    ax, 2                   
  00ab7  8b76fe             mov    si, word ptr [bp - 2]   
  00aba  8904               mov    word ptr [si], ax       
  00abc  eb02               jmp    0xac0                   
  00abe  eb60               jmp    0xb20                   
  00ac0  8b36bda8           mov    si, word ptr [0xa8bd]   
  00ac4  0376f6             add    si, word ptr [bp - 0xa] 
  00ac7  8a04               mov    al, byte ptr [si]       
  00ac9  98                 cwde                           
  00aca  beda9c             mov    si, 0x9cda              
  00acd  8b5604             mov    dx, word ptr [bp + 4]   
  00ad0  d1e2               shl    dx, 1                   
  00ad2  03f2               add    si, dx                  
  00ad4  8904               mov    word ptr [si], ax       
  00ad6  8b76fe             mov    si, word ptr [bp - 2]   
  00ad9  8b04               mov    ax, word ptr [si]       
  00adb  83f802             cmp    ax, 2                   
  00ade  7513               jne    0xaf3                   
  00ae0  b80200             mov    ax, 2                   
  00ae3  beda9c             mov    si, 0x9cda              
  00ae6  8b5604             mov    dx, word ptr [bp + 4]   
  00ae9  d1e2               shl    dx, 1                   
  00aeb  03f2               add    si, dx                  
  00aed  8b14               mov    dx, word ptr [si]       
  00aef  2bd0               sub    dx, ax                  
  00af1  8914               mov    word ptr [si], dx       
  00af3  8b36bfa8           mov    si, word ptr [0xa8bf]   
  00af7  0376f6             add    si, word ptr [bp - 0xa] 
  00afa  8a04               mov    al, byte ptr [si]       
  00afc  98                 cwde                           
  00afd  be129d             mov    si, 0x9d12              
  00b00  8b5604             mov    dx, word ptr [bp + 4]   
  00b03  d1e2               shl    dx, 1                   
  00b05  03f2               add    si, dx                  
  00b07  8904               mov    word ptr [si], ax       
  00b09  33c0               xor    ax, ax                  
  00b0b  bea29c             mov    si, 0x9ca2              
  00b0e  8b5604             mov    dx, word ptr [bp + 4]   
  00b11  d1e2               shl    dx, 1                   
  00b13  03f2               add    si, dx                  
  00b15  8904               mov    word ptr [si], ax       
  00b17  ff46f6             inc    word ptr [bp - 0xa]     
  00b1a  ff4604             inc    word ptr [bp + 4]       
  00b1d  e9e2fe             jmp    0xa02                   
  00b20  8b4604             mov    ax, word ptr [bp + 4]   
  00b23  8be5               mov    sp, bp                  
  00b25  5d                 pop    bp                      
  00b26  c3                 ret                            
  00b27  55                 push   bp                      
  00b28  8bec               mov    bp, sp                  
  00b2a  83ec04             sub    sp, 4                   
  00b2d  e8a500             call   0xbd5                   
  00b30  33c0               xor    ax, ax                  
  00b32  8946fc             mov    word ptr [bp - 4], ax   
  00b35  817efc1400         cmp    word ptr [bp - 4], 0x14 
  00b3a  7c03               jl     0xb3f                   
  00b3c  e99200             jmp    0xbd1                   
  00b3f  bedc9f             mov    si, 0x9fdc              
  00b42  0376fc             add    si, word ptr [bp - 4]   
  00b45  8a04               mov    al, byte ptr [si]       
  00b47  98                 cwde                           
  00b48  0bc0               or     ax, ax                  
  00b4a  742c               je     0xb78                   
  00b4c  bea09f             mov    si, 0x9fa0              
  00b4f  0376fc             add    si, word ptr [bp - 4]   
  00b52  8a04               mov    al, byte ptr [si]       
  00b54  98                 cwde                           
  00b55  09c0               or     ax, ax                  
  00b57  751f               jne    0xb78                   
  00b59  beb49f             mov    si, 0x9fb4              
  00b5c  0376fc             add    si, word ptr [bp - 4]   
  00b5f  8a04               mov    al, byte ptr [si]       
  00b61  98                 cwde                           
  00b62  09c0               or     ax, ax                  
  00b64  7512               jne    0xb78                   
  00b66  bec89f             mov    si, 0x9fc8              
  00b69  0376fc             add    si, word ptr [bp - 4]   
  00b6c  8a04               mov    al, byte ptr [si]       
  00b6e  98                 cwde                           
  00b6f  09c0               or     ax, ax                  
  00b71  7505               jne    0xb78                   
  00b73  b80200             mov    ax, 2                   
  00b76  eb03               jmp    0xb7b                   
  00b78  b80100             mov    ax, 1                   
  00b7b  be90a0             mov    si, 0xa090              
  00b7e  8b56fc             mov    dx, word ptr [bp - 4]   
  00b81  d1e2               shl    dx, 1                   
  00b83  03f2               add    si, dx                  
  00b85  8904               mov    word ptr [si], ax       
  00b87  bef09f             mov    si, 0x9ff0              
  00b8a  0376fc             add    si, word ptr [bp - 4]   
  00b8d  8a04               mov    al, byte ptr [si]       
  00b8f  98                 cwde                           
  00b90  09c0               or     ax, ax                  
  00b92  7518               jne    0xbac                   
  00b94  ff76fc             push   word ptr [bp - 4]       
  00b97  e8aa00             call   0xc44                   
  00b9a  5f                 pop    di                      
  00b9b  b80f27             mov    ax, 0x270f              
  00b9e  beb09e             mov    si, 0x9eb0              
  00ba1  8b56fc             mov    dx, word ptr [bp - 4]   
  00ba4  d1e2               shl    dx, 1                   
  00ba6  03f2               add    si, dx                  
  00ba8  8904               mov    word ptr [si], ax       
  00baa  eb1f               jmp    0xbcb                   
  00bac  beb09e             mov    si, 0x9eb0              
  00baf  8b46fc             mov    ax, word ptr [bp - 4]   
  00bb2  d1e0               shl    ax, 1                   
  00bb4  03f0               add    si, ax                  
  00bb6  8b04               mov    ax, word ptr [si]       
  00bb8  3d0f27             cmp    ax, 0x270f              
  00bbb  750e               jne    0xbcb                   
  00bbd  33c0               xor    ax, ax                  
  00bbf  beb09e             mov    si, 0x9eb0              
  00bc2  8b56fc             mov    dx, word ptr [bp - 4]   
  00bc5  d1e2               shl    dx, 1                   
  00bc7  03f2               add    si, dx                  
  00bc9  8904               mov    word ptr [si], ax       
  00bcb  ff46fc             inc    word ptr [bp - 4]       
  00bce  e964ff             jmp    0xb35                   
  00bd1  8be5               mov    sp, bp                  
  00bd3  5d                 pop    bp                      
  00bd4  c3                 ret                            
  00bd5  55                 push   bp                      
  00bd6  8bec               mov    bp, sp                  
  00bd8  83ec02             sub    sp, 2                   
  00bdb  33c0               xor    ax, ax                  
  00bdd  8946fe             mov    word ptr [bp - 2], ax   
  00be0  817efe1400         cmp    word ptr [bp - 2], 0x14 
  00be5  7d59               jge    0xc40                   
  00be7  bea09f             mov    si, 0x9fa0              
  00bea  0376fe             add    si, word ptr [bp - 2]   
  00bed  8a04               mov    al, byte ptr [si]       
  00bef  98                 cwde                           
  00bf0  50                 push   ax                      
  00bf1  beb49f             mov    si, 0x9fb4              
  00bf4  0376fe             add    si, word ptr [bp - 2]   
  00bf7  8a04               mov    al, byte ptr [si]       
  00bf9  98                 cwde                           
  00bfa  5a                 pop    dx                      
  00bfb  03d0               add    dx, ax                  
  00bfd  bec89f             mov    si, 0x9fc8              
  00c00  0376fe             add    si, word ptr [bp - 2]   
  00c03  8a04               mov    al, byte ptr [si]       
  00c05  98                 cwde                           
  00c06  03d0               add    dx, ax                  
  00c08  bedc9f             mov    si, 0x9fdc              
  00c0b  0376fe             add    si, word ptr [bp - 2]   
  00c0e  8a04               mov    al, byte ptr [si]       
  00c10  98                 cwde                           
  00c11  03d0               add    dx, ax                  
  00c13  bef09f             mov    si, 0x9ff0              
  00c16  0376fe             add    si, word ptr [bp - 2]   
  00c19  8814               mov    byte ptr [si], dl       
  00c1b  bef09f             mov    si, 0x9ff0              
  00c1e  0376fe             add    si, word ptr [bp - 2]   
  00c21  8a04               mov    al, byte ptr [si]       
  00c23  98                 cwde                           
  00c24  09c0               or     ax, ax                  
  00c26  7d13               jge    0xc3b                   
  00c28  33c0               xor    ax, ax                  
  00c2a  bedc9f             mov    si, 0x9fdc              
  00c2d  0376fe             add    si, word ptr [bp - 2]   
  00c30  8804               mov    byte ptr [si], al       
  00c32  98                 cwde                           
  00c33  bef09f             mov    si, 0x9ff0              
  00c36  0376fe             add    si, word ptr [bp - 2]   
  00c39  8804               mov    byte ptr [si], al       
  00c3b  ff46fe             inc    word ptr [bp - 2]       
  00c3e  eba0               jmp    0xbe0                   
  00c40  8be5               mov    sp, bp                  
  00c42  5d                 pop    bp                      
  00c43  c3                 ret                            
  00c44  55                 push   bp                      
  00c45  8bec               mov    bp, sp                  
  00c47  83ec02             sub    sp, 2                   
  00c4a  be94a1             mov    si, 0xa194              
  00c4d  8b4604             mov    ax, word ptr [bp + 4]   
  00c50  d1e0               shl    ax, 1                   
  00c52  03f0               add    si, ax                  
  00c54  8b04               mov    ax, word ptr [si]       
  00c56  0bc0               or     ax, ax                  
  00c58  745c               je     0xcb6                   
  00c5a  be44a1             mov    si, 0xa144              
  00c5d  8b4604             mov    ax, word ptr [bp + 4]   
  00c60  d1e0               shl    ax, 1                   
  00c62  03f0               add    si, ax                  
  00c64  8b04               mov    ax, word ptr [si]       
  00c66  8946fe             mov    word ptr [bp - 2], ax   
  00c69  b80100             mov    ax, 1                   
  00c6c  0bc0               or     ax, ax                  
  00c6e  7429               je     0xc99                   
  00c70  33c0               xor    ax, ax                  
  00c72  be8297             mov    si, 0x9782              
  00c75  0376fe             add    si, word ptr [bp - 2]   
  00c78  8804               mov    byte ptr [si], al       
  00c7a  ff46fe             inc    word ptr [bp - 2]       
  00c7d  8b46fe             mov    ax, word ptr [bp - 2]   
  00c80  3de703             cmp    ax, 0x3e7               
  00c83  7e05               jle    0xc8a                   
  00c85  33c0               xor    ax, ax                  
  00c87  8946fe             mov    word ptr [bp - 2], ax   
  00c8a  be8297             mov    si, 0x9782              
  00c8d  0376fe             add    si, word ptr [bp - 2]   
  00c90  8a04               mov    al, byte ptr [si]       
  00c92  98                 cwde                           
  00c93  09c0               or     ax, ax                  
  00c95  7402               je     0xc99                   
  00c97  ebd0               jmp    0xc69                   
  00c99  b80f27             mov    ax, 0x270f              
  00c9c  be1ea5             mov    si, 0xa51e              
  00c9f  8b5604             mov    dx, word ptr [bp + 4]   
  00ca2  d1e2               shl    dx, 1                   
  00ca4  03f2               add    si, dx                  
  00ca6  8904               mov    word ptr [si], ax       
  00ca8  33c0               xor    ax, ax                  
  00caa  be94a1             mov    si, 0xa194              
  00cad  8b5604             mov    dx, word ptr [bp + 4]   
  00cb0  d1e2               shl    dx, 1                   
  00cb2  03f2               add    si, dx                  
  00cb4  8904               mov    word ptr [si], ax       
  00cb6  bebca1             mov    si, 0xa1bc              
  00cb9  8b4604             mov    ax, word ptr [bp + 4]   
  00cbc  d1e0               shl    ax, 1                   
  00cbe  03f0               add    si, ax                  
  00cc0  8b04               mov    ax, word ptr [si]       
  00cc2  0bc0               or     ax, ax                  
  00cc4  744a               je     0xd10                   
  00cc6  be6ca1             mov    si, 0xa16c              
  00cc9  8b4604             mov    ax, word ptr [bp + 4]   
  00ccc  d1e0               shl    ax, 1                   
  00cce  03f0               add    si, ax                  
  00cd0  8b04               mov    ax, word ptr [si]       
  00cd2  8946fe             mov    word ptr [bp - 2], ax   
  00cd5  b80100             mov    ax, 1                   
  00cd8  0bc0               or     ax, ax                  
  00cda  7426               je     0xd02                   
  00cdc  33c0               xor    ax, ax                  
  00cde  be6a9b             mov    si, 0x9b6a              
  00ce1  0376fe             add    si, word ptr [bp - 2]   
  00ce4  8804               mov    byte ptr [si], al       
  00ce6  ff46fe             inc    word ptr [bp - 2]       
  00ce9  8b46fe             mov    ax, word ptr [bp - 2]   
  00cec  3dc700             cmp    ax, 0xc7                
  00cef  7f0d               jg     0xcfe                   
  00cf1  be6a9b             mov    si, 0x9b6a              
  00cf4  0376fe             add    si, word ptr [bp - 2]   
  00cf7  8a04               mov    al, byte ptr [si]       
  00cf9  98                 cwde                           
  00cfa  09c0               or     ax, ax                  
  00cfc  7502               jne    0xd00                   
  00cfe  eb02               jmp    0xd02                   
  00d00  ebd3               jmp    0xcd5                   
  00d02  33c0               xor    ax, ax                  
  00d04  bebca1             mov    si, 0xa1bc              
  00d07  8b5604             mov    dx, word ptr [bp + 4]   
  00d0a  d1e2               shl    dx, 1                   
  00d0c  03f2               add    si, dx                  
  00d0e  8904               mov    word ptr [si], ax       
  00d10  beb09e             mov    si, 0x9eb0              
  00d13  8b4604             mov    ax, word ptr [bp + 4]   
  00d16  d1e0               shl    ax, 1                   
  00d18  03f0               add    si, ax                  
  00d1a  8b04               mov    ax, word ptr [si]       
  00d1c  3d0f27             cmp    ax, 0x270f              
  00d1f  741a               je     0xd3b                   
  00d21  33c0               xor    ax, ax                  
  00d23  bed9a8             mov    si, 0xa8d9              
  00d26  8b5604             mov    dx, word ptr [bp + 4]   
  00d29  d1e2               shl    dx, 1                   
  00d2b  03f2               add    si, dx                  
  00d2d  8904               mov    word ptr [si], ax       
  00d2f  beb09e             mov    si, 0x9eb0              
  00d32  8b5604             mov    dx, word ptr [bp + 4]   
  00d35  d1e2               shl    dx, 1                   
  00d37  03f2               add    si, dx                  
  00d39  8904               mov    word ptr [si], ax       
  00d3b  8be5               mov    sp, bp                  
  00d3d  5d                 pop    bp                      
  00d3e  c3                 ret                            
  00d3f  55                 push   bp                      
  00d40  8bec               mov    bp, sp                  
  00d42  83ec0a             sub    sp, 0xa                 
  00d45  bb0e00             mov    bx, 0xe                 
  00d48  be289f             mov    si, 0x9f28              
  00d4b  033606a5           add    si, word ptr [0xa506]   
  00d4f  8a04               mov    al, byte ptr [si]       
  00d51  98                 cwde                           
  00d52  f7eb               imul   bx                      
  00d54  8946f8             mov    word ptr [bp - 8], ax   
  00d57  b817a7             mov    ax, 0xa717              
  00d5a  0346f8             add    ax, word ptr [bp - 8]   
  00d5d  8906bba8           mov    word ptr [0xa8bb], ax   
  00d61  b8a3a7             mov    ax, 0xa7a3              
  00d64  0346f8             add    ax, word ptr [bp - 8]   
  00d67  8906bda8           mov    word ptr [0xa8bd], ax   
  00d6b  b82fa8             mov    ax, 0xa82f              
  00d6e  0346f8             add    ax, word ptr [bp - 8]   
  00d71  8906bfa8           mov    word ptr [0xa8bf], ax   
  00d75  8b4604             mov    ax, word ptr [bp + 4]   
  00d78  0bc0               or     ax, ax                  
  00d7a  7422               je     0xd9e                   
  00d7c  b81c00             mov    ax, 0x1c                
  00d7f  50                 push   ax                      
  00d80  b8da9c             mov    ax, 0x9cda              
  00d83  50                 push   ax                      
  00d84  b8ffff             mov    ax, 0xffff              
  00d87  50                 push   ax                      
  00d88  e8a111             call   0x1f2c                  
  00d8b  83c406             add    sp, 6                   
  00d8e  33c0               xor    ax, ax                  
  00d90  50                 push   ax                      
  00d91  ff3606a5           push   word ptr [0xa506]       
  00d95  33c0               xor    ax, ax                  
  00d97  50                 push   ax                      
  00d98  e838fc             call   0x9d3                   
  00d9b  83c406             add    sp, 6                   
  00d9e  e8c70f             call   0x1d68                  
  00da1  b86800             mov    ax, 0x68                
  00da4  50                 push   ax                      
  00da5  b056               mov    al, 0x56                
  00da7  50                 push   ax                      
  00da8  b011               mov    al, 0x11                
  00daa  50                 push   ax                      
  00dab  e82310             call   0x1dd1                  
  00dae  83c406             add    sp, 6                   
  00db1  b81200             mov    ax, 0x12                
  00db4  50                 push   ax                      
  00db5  b056               mov    al, 0x56                
  00db7  50                 push   ax                      
  00db8  33c0               xor    ax, ax                  
  00dba  50                 push   ax                      
  00dbb  e83510             call   0x1df3                  
  00dbe  83c406             add    sp, 6                   
  00dc1  b81200             mov    ax, 0x12                
  00dc4  50                 push   ax                      
  00dc5  b0be               mov    al, 0xbe                
  00dc7  50                 push   ax                      
  00dc8  33c0               xor    ax, ax                  
  00dca  50                 push   ax                      
  00dcb  e82510             call   0x1df3                  
  00dce  83c406             add    sp, 6                   
  00dd1  33c0               xor    ax, ax                  
  00dd3  8946f8             mov    word ptr [bp - 8], ax   
  00dd6  817ef80e00         cmp    word ptr [bp - 8], 0xe  
  00ddb  7c03               jl     0xde0                   
  00ddd  e9d900             jmp    0xeb9                   
  00de0  beda9c             mov    si, 0x9cda              
  00de3  8b46f8             mov    ax, word ptr [bp - 8]   
  00de6  d1e0               shl    ax, 1                   
  00de8  03f0               add    si, ax                  
  00dea  8b04               mov    ax, word ptr [si]       
  00dec  83f8ff             cmp    ax, -1                  
  00def  7503               jne    0xdf4                   
  00df1  e9bf00             jmp    0xeb3                   
  00df4  beda9c             mov    si, 0x9cda              
  00df7  8b46f8             mov    ax, word ptr [bp - 8]   
  00dfa  d1e0               shl    ax, 1                   
  00dfc  03f0               add    si, ax                  
  00dfe  8b04               mov    ax, word ptr [si]       
  00e00  8946fa             mov    word ptr [bp - 6], ax   
  00e03  be329c             mov    si, 0x9c32              
  00e06  8b46f8             mov    ax, word ptr [bp - 8]   
  00e09  d1e0               shl    ax, 1                   
  00e0b  03f0               add    si, ax                  
  00e0d  8b04               mov    ax, word ptr [si]       
  00e0f  83f802             cmp    ax, 2                   
  00e12  7505               jne    0xe19                   
  00e14  8146fa0200         add    word ptr [bp - 6], 2    
  00e19  8b46fa             mov    ax, word ptr [bp - 6]   
  00e1c  d1f8               sar    ax, 1                   
  00e1e  83e816             sub    ax, 0x16                
  00e21  8946fa             mov    word ptr [bp - 6], ax   
  00e24  be129d             mov    si, 0x9d12              
  00e27  8b46f8             mov    ax, word ptr [bp - 8]   
  00e2a  d1e0               shl    ax, 1                   
  00e2c  03f0               add    si, ax                  
  00e2e  8b04               mov    ax, word ptr [si]       
  00e30  83c010             add    ax, 0x10                
  00e33  8946fc             mov    word ptr [bp - 4], ax   
  00e36  be329c             mov    si, 0x9c32              
  00e39  8b46f8             mov    ax, word ptr [bp - 8]   
  00e3c  d1e0               shl    ax, 1                   
  00e3e  03f0               add    si, ax                  
  00e40  8b04               mov    ax, word ptr [si]       
  00e42  09c0               or     ax, ax                  
  00e44  7508               jne    0xe4e                   
  00e46  b8dc03             mov    ax, 0x3dc               
  00e49  8946f6             mov    word ptr [bp - 0xa], ax 
  00e4c  eb1f               jmp    0xe6d                   
  00e4e  be329c             mov    si, 0x9c32              
  00e51  8b46f8             mov    ax, word ptr [bp - 8]   
  00e54  d1e0               shl    ax, 1                   
  00e56  03f0               add    si, ax                  
  00e58  8b04               mov    ax, word ptr [si]       
  00e5a  83f801             cmp    ax, 1                   
  00e5d  7508               jne    0xe67                   
  00e5f  b8d003             mov    ax, 0x3d0               
  00e62  8946f6             mov    word ptr [bp - 0xa], ax 
  00e65  eb06               jmp    0xe6d                   
  00e67  b8c403             mov    ax, 0x3c4               
  00e6a  8946f6             mov    word ptr [bp - 0xa], ax 
  00e6d  817efa0f00         cmp    word ptr [bp - 6], 0xf  
  00e72  7e06               jle    0xe7a                   
  00e74  b80f00             mov    ax, 0xf                 
  00e77  8946fa             mov    word ptr [bp - 6], ax   
  00e7a  817efa0100         cmp    word ptr [bp - 6], 1    
  00e7f  7d06               jge    0xe87                   
  00e81  b80100             mov    ax, 1                   
  00e84  8946fa             mov    word ptr [bp - 6], ax   
  00e87  817efc5c00         cmp    word ptr [bp - 4], 0x5c 
  00e8c  7e06               jle    0xe94                   
  00e8e  b85c00             mov    ax, 0x5c                
  00e91  8946fc             mov    word ptr [bp - 4], ax   
  00e94  817efc2c00         cmp    word ptr [bp - 4], 0x2c 
  00e99  7d06               jge    0xea1                   
  00e9b  b82c00             mov    ax, 0x2c                
  00e9e  8946fc             mov    word ptr [bp - 4], ax   
  00ea1  ff76f6             push   word ptr [bp - 0xa]     
  00ea4  8b46fc             mov    ax, word ptr [bp - 4]   
  00ea7  d1e0               shl    ax, 1                   
  00ea9  50                 push   ax                      
  00eaa  ff76fa             push   word ptr [bp - 6]       
  00ead  e8620f             call   0x1e12                  
  00eb0  83c406             add    sp, 6                   
  00eb3  ff46f8             inc    word ptr [bp - 8]       
  00eb6  e91dff             jmp    0xdd6                   
  00eb9  8b4604             mov    ax, word ptr [bp + 4]   
  00ebc  0bc0               or     ax, ax                  
  00ebe  742b               je     0xeeb                   
  00ec0  b81700             mov    ax, 0x17                
  00ec3  50                 push   ax                      
  00ec4  b015               mov    al, 0x15                
  00ec6  50                 push   ax                      
  00ec7  e89211             call   0x205c                  
  00eca  5f                 pop    di                      
  00ecb  5f                 pop    di                      
  00ecc  b83900             mov    ax, 0x39                
  00ecf  50                 push   ax                      
  00ed0  e89216             call   0x2565                  
  00ed3  5f                 pop    di                      
  00ed4  be289f             mov    si, 0x9f28              
  00ed7  033606a5           add    si, word ptr [0xa506]   
  00edb  8a04               mov    al, byte ptr [si]       
  00edd  98                 cwde                           
  00ede  50                 push   ax                      
  00edf  e88908             call   0x176b                  
  00ee2  5f                 pop    di                      
  00ee3  b82000             mov    ax, 0x20                
  00ee6  50                 push   ax                      
  00ee7  e8fd15             call   0x24e7                  
  00eea  5f                 pop    di                      
  00eeb  8be5               mov    sp, bp                  
  00eed  5d                 pop    bp                      
  00eee  c3                 ret                            
  00eef  55                 push   bp                      
  00ef0  8bec               mov    bp, sp                  
  00ef2  83ec02             sub    sp, 2                   
  00ef5  e8140a             call   0x190c                  
  00ef8  e8a20b             call   0x1a9d                  
  00efb  e89b0a             call   0x1999                  
  00efe  b81300             mov    ax, 0x13                
  00f01  8946fe             mov    word ptr [bp - 2], ax   
  00f04  817efe0000         cmp    word ptr [bp - 2], 0    
  00f09  7c1d               jl     0xf28                   
  00f0b  be1ea5             mov    si, 0xa51e              
  00f0e  8b46fe             mov    ax, word ptr [bp - 2]   
  00f11  d1e0               shl    ax, 1                   
  00f13  03f0               add    si, ax                  
  00f15  8b04               mov    ax, word ptr [si]       
  00f17  3d0f27             cmp    ax, 0x270f              
  00f1a  7407               je     0xf23                   
  00f1c  ff76fe             push   word ptr [bp - 2]       
  00f1f  e8710d             call   0x1c93                  
  00f22  5f                 pop    di                      
  00f23  ff4efe             dec    word ptr [bp - 2]       
  00f26  ebdc               jmp    0xf04                   
  00f28  8be5               mov    sp, bp                  
  00f2a  5d                 pop    bp                      
  00f2b  c3                 ret                            
  00f2c  55                 push   bp                      
  00f2d  8bec               mov    bp, sp                  
  00f2f  83ec02             sub    sp, 2                   
  00f32  33c0               xor    ax, ax                  
  00f34  8946fe             mov    word ptr [bp - 2], ax   
  00f37  817efe1400         cmp    word ptr [bp - 2], 0x14 
  00f3c  7d24               jge    0xf62                   
  00f3e  be1ea5             mov    si, 0xa51e              
  00f41  8b46fe             mov    ax, word ptr [bp - 2]   
  00f44  d1e0               shl    ax, 1                   
  00f46  03f0               add    si, ax                  
  00f48  8b04               mov    ax, word ptr [si]       
  00f4a  3d0f27             cmp    ax, 0x270f              
  00f4d  740e               je     0xf5d                   
  00f4f  ff76fe             push   word ptr [bp - 2]       
  00f52  e8120d             call   0x1c67                  
  00f55  5f                 pop    di                      
  00f56  ff76fe             push   word ptr [bp - 2]       
  00f59  e8600d             call   0x1cbc                  
  00f5c  5f                 pop    di                      
  00f5d  ff46fe             inc    word ptr [bp - 2]       
  00f60  ebd5               jmp    0xf37                   
  00f62  e8010a             call   0x1966                  
  00f65  e8710a             call   0x19d9                  
  00f68  e8fb0a             call   0x1a66                  
  00f6b  e8720b             call   0x1ae0                  
  00f6e  e8c508             call   0x1836                  
  00f71  813e2fa96300       cmp    word ptr [0xa92f], 0x63 
  00f77  7403               je     0xf7c                   
  00f79  e82d09             call   0x18a9                  
  00f7c  8be5               mov    sp, bp                  
  00f7e  5d                 pop    bp                      
  00f7f  c3                 ret                            
  00f80  55                 push   bp                      
  00f81  8bec               mov    bp, sp                  
  00f83  813ec0a50200       cmp    word ptr [0xa5c0], 2    
  00f89  7e0c               jle    0xf97                   
  00f8b  b80200             mov    ax, 2                   
  00f8e  2b06c0a5           sub    ax, word ptr [0xa5c0]   
  00f92  50                 push   ax                      
  00f93  e8c5f5             call   0x55b                   
  00f96  5f                 pop    di                      
  00f97  b81700             mov    ax, 0x17                
  00f9a  50                 push   ax                      
  00f9b  e8400f             call   0x1ede                  
  00f9e  8be5               mov    sp, bp                  
  00fa0  5d                 pop    bp                      
  00fa1  c3                 ret                            
  00fa2  55                 push   bp                      
  00fa3  8bec               mov    bp, sp                  
  00fa5  8b4604             mov    ax, word ptr [bp + 4]   
  00fa8  83f80c             cmp    ax, 0xc                 
  00fab  b80100             mov    ax, 1                   
  00fae  7402               je     0xfb2                   
  00fb0  ffc8               dec    ax                      
  00fb2  0bc0               or     ax, ax                  
  00fb4  7515               jne    0xfcb                   
  00fb6  8b4604             mov    ax, word ptr [bp + 4]   
  00fb9  83f84a             cmp    ax, 0x4a                
  00fbc  b80100             mov    ax, 1                   
  00fbf  7402               je     0xfc3                   
  00fc1  ffc8               dec    ax                      
  00fc3  0bc0               or     ax, ax                  
  00fc5  7504               jne    0xfcb                   
  00fc7  31c0               xor    ax, ax                  
  00fc9  eb03               jmp    0xfce                   
  00fcb  b80100             mov    ax, 1                   
  00fce  0bc0               or     ax, ax                  
  00fd0  7403               je     0xfd5                   
  00fd2  b80100             mov    ax, 1                   
  00fd5  0bc0               or     ax, ax                  
  00fd7  7515               jne    0xfee                   
  00fd9  8b4604             mov    ax, word ptr [bp + 4]   
  00fdc  83f80f             cmp    ax, 0xf                 
  00fdf  b80100             mov    ax, 1                   
  00fe2  7402               je     0xfe6                   
  00fe4  ffc8               dec    ax                      
  00fe6  0bc0               or     ax, ax                  
  00fe8  7504               jne    0xfee                   
  00fea  31c0               xor    ax, ax                  
  00fec  eb03               jmp    0xff1                   
  00fee  b80100             mov    ax, 1                   
  00ff1  8be5               mov    sp, bp                  
  00ff3  5d                 pop    bp                      
  00ff4  c3                 ret                            
  00ff5  55                 push   bp                      
  00ff6  8bec               mov    bp, sp                  
  00ff8  8b4604             mov    ax, word ptr [bp + 4]   
  00ffb  83f80d             cmp    ax, 0xd                 
  00ffe  b80100             mov    ax, 1                   
  01001  7402               je     0x1005                  
  01003  ffc8               dec    ax                      
  01005  0bc0               or     ax, ax                  
  01007  7515               jne    0x101e                  
  01009  8b4604             mov    ax, word ptr [bp + 4]   
  0100c  83f84e             cmp    ax, 0x4e                
  0100f  b80100             mov    ax, 1                   
  01012  7402               je     0x1016                  
  01014  ffc8               dec    ax                      
  01016  0bc0               or     ax, ax                  
  01018  7504               jne    0x101e                  
  0101a  31c0               xor    ax, ax                  
  0101c  eb03               jmp    0x1021                  
  0101e  b80100             mov    ax, 1                   
  01021  0bc0               or     ax, ax                  
  01023  7403               je     0x1028                  
  01025  b80100             mov    ax, 1                   
  01028  0bc0               or     ax, ax                  
  0102a  7515               jne    0x1041                  
  0102c  8b4604             mov    ax, word ptr [bp + 4]   
  0102f  83f81d             cmp    ax, 0x1d                
  01032  b80100             mov    ax, 1                   
  01035  7402               je     0x1039                  
  01037  ffc8               dec    ax                      
  01039  0bc0               or     ax, ax                  
  0103b  7504               jne    0x1041                  
  0103d  31c0               xor    ax, ax                  
  0103f  eb03               jmp    0x1044                  
  01041  b80100             mov    ax, 1                   
  01044  8be5               mov    sp, bp                  
  01046  5d                 pop    bp                      
  01047  c3                 ret                            
  01048  55                 push   bp                      
  01049  8bec               mov    bp, sp                  
  0104b  8b4604             mov    ax, word ptr [bp + 4]   
  0104e  83f825             cmp    ax, 0x25                
  01051  b80100             mov    ax, 1                   
  01054  7d02               jge    0x1058                  
  01056  ffc8               dec    ax                      
  01058  0bc0               or     ax, ax                  
  0105a  7403               je     0x105f                  
  0105c  b80100             mov    ax, 1                   
  0105f  0bc0               or     ax, ax                  
  01061  741d               je     0x1080                  
  01063  8b4604             mov    ax, word ptr [bp + 4]   
  01066  83f836             cmp    ax, 0x36                
  01069  b80100             mov    ax, 1                   
  0106c  7e02               jle    0x1070                  
  0106e  ffc8               dec    ax                      
  01070  0bc0               or     ax, ax                  
  01072  7403               je     0x1077                  
  01074  b80100             mov    ax, 1                   
  01077  0bc0               or     ax, ax                  
  01079  7405               je     0x1080                  
  0107b  b80100             mov    ax, 1                   
  0107e  eb02               jmp    0x1082                  
  01080  31c0               xor    ax, ax                  
  01082  0bc0               or     ax, ax                  
  01084  7403               je     0x1089                  
  01086  b80100             mov    ax, 1                   
  01089  0bc0               or     ax, ax                  
  0108b  7515               jne    0x10a2                  
  0108d  8b4604             mov    ax, word ptr [bp + 4]   
  01090  83f841             cmp    ax, 0x41                
  01093  b80100             mov    ax, 1                   
  01096  7402               je     0x109a                  
  01098  ffc8               dec    ax                      
  0109a  0bc0               or     ax, ax                  
  0109c  7504               jne    0x10a2                  
  0109e  31c0               xor    ax, ax                  
  010a0  eb03               jmp    0x10a5                  
  010a2  b80100             mov    ax, 1                   
  010a5  0bc0               or     ax, ax                  
  010a7  7403               je     0x10ac                  
  010a9  b80100             mov    ax, 1                   
  010ac  0bc0               or     ax, ax                  
  010ae  7515               jne    0x10c5                  
  010b0  8b4604             mov    ax, word ptr [bp + 4]   
  010b3  83f842             cmp    ax, 0x42                
  010b6  b80100             mov    ax, 1                   
  010b9  7402               je     0x10bd                  
  010bb  ffc8               dec    ax                      
  010bd  0bc0               or     ax, ax                  
  010bf  7504               jne    0x10c5                  
  010c1  31c0               xor    ax, ax                  
  010c3  eb03               jmp    0x10c8                  
  010c5  b80100             mov    ax, 1                   
  010c8  0bc0               or     ax, ax                  
  010ca  7403               je     0x10cf                  
  010cc  b80100             mov    ax, 1                   
  010cf  0bc0               or     ax, ax                  
  010d1  7515               jne    0x10e8                  
  010d3  8b4604             mov    ax, word ptr [bp + 4]   
  010d6  83f847             cmp    ax, 0x47                
  010d9  b80100             mov    ax, 1                   
  010dc  7402               je     0x10e0                  
  010de  ffc8               dec    ax                      
  010e0  0bc0               or     ax, ax                  
  010e2  7504               jne    0x10e8                  
  010e4  31c0               xor    ax, ax                  
  010e6  eb03               jmp    0x10eb                  
  010e8  b80100             mov    ax, 1                   
  010eb  0bc0               or     ax, ax                  
  010ed  7403               je     0x10f2                  
  010ef  b80100             mov    ax, 1                   
  010f2  0bc0               or     ax, ax                  
  010f4  7515               jne    0x110b                  
  010f6  8b4604             mov    ax, word ptr [bp + 4]   
  010f9  83f848             cmp    ax, 0x48                
  010fc  b80100             mov    ax, 1                   
  010ff  7402               je     0x1103                  
  01101  ffc8               dec    ax                      
  01103  0bc0               or     ax, ax                  
  01105  7504               jne    0x110b                  
  01107  31c0               xor    ax, ax                  
  01109  eb03               jmp    0x110e                  
  0110b  b80100             mov    ax, 1                   
  0110e  8be5               mov    sp, bp                  
  01110  5d                 pop    bp                      
  01111  c3                 ret                            
  01112  55                 push   bp                      
  01113  8bec               mov    bp, sp                  
  01115  8b4604             mov    ax, word ptr [bp + 4]   
  01118  83f837             cmp    ax, 0x37                
  0111b  b80100             mov    ax, 1                   
  0111e  7d02               jge    0x1122                  
  01120  ffc8               dec    ax                      
  01122  0bc0               or     ax, ax                  
  01124  7403               je     0x1129                  
  01126  b80100             mov    ax, 1                   
  01129  0bc0               or     ax, ax                  
  0112b  741d               je     0x114a                  
  0112d  8b4604             mov    ax, word ptr [bp + 4]   
  01130  83f839             cmp    ax, 0x39                
  01133  b80100             mov    ax, 1                   
  01136  7e02               jle    0x113a                  
  01138  ffc8               dec    ax                      
  0113a  0bc0               or     ax, ax                  
  0113c  7403               je     0x1141                  
  0113e  b80100             mov    ax, 1                   
  01141  0bc0               or     ax, ax                  
  01143  7405               je     0x114a                  
  01145  b80100             mov    ax, 1                   
  01148  eb02               jmp    0x114c                  
  0114a  31c0               xor    ax, ax                  
  0114c  0bc0               or     ax, ax                  
  0114e  7403               je     0x1153                  
  01150  b80100             mov    ax, 1                   
  01153  0bc0               or     ax, ax                  
  01155  7546               jne    0x119d                  
  01157  8b4604             mov    ax, word ptr [bp + 4]   
  0115a  83f83d             cmp    ax, 0x3d                
  0115d  b80100             mov    ax, 1                   
  01160  7d02               jge    0x1164                  
  01162  ffc8               dec    ax                      
  01164  0bc0               or     ax, ax                  
  01166  7403               je     0x116b                  
  01168  b80100             mov    ax, 1                   
  0116b  0bc0               or     ax, ax                  
  0116d  741d               je     0x118c                  
  0116f  8b4604             mov    ax, word ptr [bp + 4]   
  01172  83f83f             cmp    ax, 0x3f                
  01175  b80100             mov    ax, 1                   
  01178  7e02               jle    0x117c                  
  0117a  ffc8               dec    ax                      
  0117c  0bc0               or     ax, ax                  
  0117e  7403               je     0x1183                  
  01180  b80100             mov    ax, 1                   
  01183  0bc0               or     ax, ax                  
  01185  7405               je     0x118c                  
  01187  b80100             mov    ax, 1                   
  0118a  eb02               jmp    0x118e                  
  0118c  31c0               xor    ax, ax                  
  0118e  0bc0               or     ax, ax                  
  01190  7403               je     0x1195                  
  01192  b80100             mov    ax, 1                   
  01195  0bc0               or     ax, ax                  
  01197  7504               jne    0x119d                  
  01199  31c0               xor    ax, ax                  
  0119b  eb03               jmp    0x11a0                  
  0119d  b80100             mov    ax, 1                   
  011a0  0bc0               or     ax, ax                  
  011a2  7403               je     0x11a7                  
  011a4  b80100             mov    ax, 1                   
  011a7  0bc0               or     ax, ax                  
  011a9  7546               jne    0x11f1                  
  011ab  8b4604             mov    ax, word ptr [bp + 4]   
  011ae  83f843             cmp    ax, 0x43                
  011b1  b80100             mov    ax, 1                   
  011b4  7d02               jge    0x11b8                  
  011b6  ffc8               dec    ax                      
  011b8  0bc0               or     ax, ax                  
  011ba  7403               je     0x11bf                  
  011bc  b80100             mov    ax, 1                   
  011bf  0bc0               or     ax, ax                  
  011c1  741d               je     0x11e0                  
  011c3  8b4604             mov    ax, word ptr [bp + 4]   
  011c6  83f845             cmp    ax, 0x45                
  011c9  b80100             mov    ax, 1                   
  011cc  7e02               jle    0x11d0                  
  011ce  ffc8               dec    ax                      
  011d0  0bc0               or     ax, ax                  
  011d2  7403               je     0x11d7                  
  011d4  b80100             mov    ax, 1                   
  011d7  0bc0               or     ax, ax                  
  011d9  7405               je     0x11e0                  
  011db  b80100             mov    ax, 1                   
  011de  eb02               jmp    0x11e2                  
  011e0  31c0               xor    ax, ax                  
  011e2  0bc0               or     ax, ax                  
  011e4  7403               je     0x11e9                  
  011e6  b80100             mov    ax, 1                   
  011e9  0bc0               or     ax, ax                  
  011eb  7504               jne    0x11f1                  
  011ed  31c0               xor    ax, ax                  
  011ef  eb03               jmp    0x11f4                  
  011f1  b80100             mov    ax, 1                   
  011f4  8be5               mov    sp, bp                  
  011f6  5d                 pop    bp                      
  011f7  c3                 ret                            
  011f8  55                 push   bp                      
  011f9  8bec               mov    bp, sp                  
  011fb  8b4604             mov    ax, word ptr [bp + 4]   
  011fe  83f805             cmp    ax, 5                   
  01201  b80100             mov    ax, 1                   
  01204  7402               je     0x1208                  
  01206  ffc8               dec    ax                      
  01208  0bc0               or     ax, ax                  
  0120a  7515               jne    0x1221                  
  0120c  8b4604             mov    ax, word ptr [bp + 4]   
  0120f  83f806             cmp    ax, 6                   
  01212  b80100             mov    ax, 1                   
  01215  7402               je     0x1219                  
  01217  ffc8               dec    ax                      
  01219  0bc0               or     ax, ax                  
  0121b  7504               jne    0x1221                  
  0121d  31c0               xor    ax, ax                  
  0121f  eb03               jmp    0x1224                  
  01221  b80100             mov    ax, 1                   
  01224  8be5               mov    sp, bp                  
  01226  5d                 pop    bp                      
  01227  c3                 ret                            
  01228  55                 push   bp                      
  01229  8bec               mov    bp, sp                  
  0122b  8b4604             mov    ax, word ptr [bp + 4]   
  0122e  83f809             cmp    ax, 9                   
  01231  b80100             mov    ax, 1                   
  01234  7402               je     0x1238                  
  01236  ffc8               dec    ax                      
  01238  0bc0               or     ax, ax                  
  0123a  7515               jne    0x1251                  
  0123c  8b4604             mov    ax, word ptr [bp + 4]   
  0123f  83f80a             cmp    ax, 0xa                 
  01242  b80100             mov    ax, 1                   
  01245  7402               je     0x1249                  
  01247  ffc8               dec    ax                      
  01249  0bc0               or     ax, ax                  
  0124b  7504               jne    0x1251                  
  0124d  31c0               xor    ax, ax                  
  0124f  eb03               jmp    0x1254                  
  01251  b80100             mov    ax, 1                   
  01254  8be5               mov    sp, bp                  
  01256  5d                 pop    bp                      
  01257  c3                 ret                            
  01258  55                 push   bp                      
  01259  8bec               mov    bp, sp                  
  0125b  8b4604             mov    ax, word ptr [bp + 4]   
  0125e  83f80d             cmp    ax, 0xd                 
  01261  b80100             mov    ax, 1                   
  01264  7d02               jge    0x1268                  
  01266  ffc8               dec    ax                      
  01268  0bc0               or     ax, ax                  
  0126a  7403               je     0x126f                  
  0126c  b80100             mov    ax, 1                   
  0126f  0bc0               or     ax, ax                  
  01271  741d               je     0x1290                  
  01273  8b4604             mov    ax, word ptr [bp + 4]   
  01276  83f80f             cmp    ax, 0xf                 
  01279  b80100             mov    ax, 1                   
  0127c  7e02               jle    0x1280                  
  0127e  ffc8               dec    ax                      
  01280  0bc0               or     ax, ax                  
  01282  7403               je     0x1287                  
  01284  b80100             mov    ax, 1                   
  01287  0bc0               or     ax, ax                  
  01289  7405               je     0x1290                  
  0128b  b80100             mov    ax, 1                   
  0128e  eb02               jmp    0x1292                  
  01290  31c0               xor    ax, ax                  
  01292  0bc0               or     ax, ax                  
  01294  7403               je     0x1299                  
  01296  b80100             mov    ax, 1                   
  01299  0bc0               or     ax, ax                  
  0129b  7546               jne    0x12e3                  
  0129d  8b4604             mov    ax, word ptr [bp + 4]   
  012a0  83f813             cmp    ax, 0x13                
  012a3  b80100             mov    ax, 1                   
  012a6  7d02               jge    0x12aa                  
  012a8  ffc8               dec    ax                      
  012aa  0bc0               or     ax, ax                  
  012ac  7403               je     0x12b1                  
  012ae  b80100             mov    ax, 1                   
  012b1  0bc0               or     ax, ax                  
  012b3  741d               je     0x12d2                  
  012b5  8b4604             mov    ax, word ptr [bp + 4]   
  012b8  83f824             cmp    ax, 0x24                
  012bb  b80100             mov    ax, 1                   
  012be  7e02               jle    0x12c2                  
  012c0  ffc8               dec    ax                      
  012c2  0bc0               or     ax, ax                  
  012c4  7403               je     0x12c9                  
  012c6  b80100             mov    ax, 1                   
  012c9  0bc0               or     ax, ax                  
  012cb  7405               je     0x12d2                  
  012cd  b80100             mov    ax, 1                   
  012d0  eb02               jmp    0x12d4                  
  012d2  31c0               xor    ax, ax                  
  012d4  0bc0               or     ax, ax                  
  012d6  7403               je     0x12db                  
  012d8  b80100             mov    ax, 1                   
  012db  0bc0               or     ax, ax                  
  012dd  7504               jne    0x12e3                  
  012df  31c0               xor    ax, ax                  
  012e1  eb03               jmp    0x12e6                  
  012e3  b80100             mov    ax, 1                   
  012e6  0bc0               or     ax, ax                  
  012e8  7403               je     0x12ed                  
  012ea  b80100             mov    ax, 1                   
  012ed  0bc0               or     ax, ax                  
  012ef  7515               jne    0x1306                  
  012f1  8b4604             mov    ax, word ptr [bp + 4]   
  012f4  83f807             cmp    ax, 7                   
  012f7  b80100             mov    ax, 1                   
  012fa  7402               je     0x12fe                  
  012fc  ffc8               dec    ax                      
  012fe  0bc0               or     ax, ax                  
  01300  7504               jne    0x1306                  
  01302  31c0               xor    ax, ax                  
  01304  eb03               jmp    0x1309                  
  01306  b80100             mov    ax, 1                   
  01309  0bc0               or     ax, ax                  
  0130b  7403               je     0x1310                  
  0130d  b80100             mov    ax, 1                   
  01310  0bc0               or     ax, ax                  
  01312  7515               jne    0x1329                  
  01314  8b4604             mov    ax, word ptr [bp + 4]   
  01317  83f808             cmp    ax, 8                   
  0131a  b80100             mov    ax, 1                   
  0131d  7402               je     0x1321                  
  0131f  ffc8               dec    ax                      
  01321  0bc0               or     ax, ax                  
  01323  7504               jne    0x1329                  
  01325  31c0               xor    ax, ax                  
  01327  eb03               jmp    0x132c                  
  01329  b80100             mov    ax, 1                   
  0132c  8be5               mov    sp, bp                  
  0132e  5d                 pop    bp                      
  0132f  c3                 ret                            
  01330  55                 push   bp                      
  01331  8bec               mov    bp, sp                  
  01333  bb0a00             mov    bx, 0xa                 
  01336  b80a00             mov    ax, 0xa                 
  01339  50                 push   ax                      
  0133a  be009f             mov    si, 0x9f00              
  0133d  8b4604             mov    ax, word ptr [bp + 4]   
  01340  d1e0               shl    ax, 1                   
  01342  03f0               add    si, ax                  
  01344  8b04               mov    ax, word ptr [si]       
  01346  83c005             add    ax, 5                   
  01349  59                 pop    cx                      
  0134a  99                 cdq                            
  0134b  f7f9               idiv   cx                      
  0134d  f7eb               imul   bx                      
  0134f  bed89e             mov    si, 0x9ed8              
  01352  8b5604             mov    dx, word ptr [bp + 4]   
  01355  d1e2               shl    dx, 1                   
  01357  03f2               add    si, dx                  
  01359  8b14               mov    dx, word ptr [si]       
  0135b  83c201             add    dx, 1                   
  0135e  b90300             mov    cx, 3                   
  01361  d3fa               sar    dx, cl                  
  01363  03c2               add    ax, dx                  
  01365  8be5               mov    sp, bp                  
  01367  5d                 pop    bp                      
  01368  c3                 ret                            
  01369  55                 push   bp                      
  0136a  8bec               mov    bp, sp                  
  0136c  ff4604             inc    word ptr [bp + 4]       
  0136f  8b4604             mov    ax, word ptr [bp + 4]   
  01372  3de703             cmp    ax, 0x3e7               
  01375  7e05               jle    0x137c                  
  01377  33c0               xor    ax, ax                  
  01379  894604             mov    word ptr [bp + 4], ax   
  0137c  8b4604             mov    ax, word ptr [bp + 4]   
  0137f  8be5               mov    sp, bp                  
  01381  5d                 pop    bp                      
  01382  c3                 ret                            
  01383  55                 push   bp                      
  01384  8bec               mov    bp, sp                  
  01386  bb0a00             mov    bx, 0xa                 
  01389  b80a00             mov    ax, 0xa                 
  0138c  50                 push   ax                      
  0138d  be789f             mov    si, 0x9f78              
  01390  8b4604             mov    ax, word ptr [bp + 4]   
  01393  d1e0               shl    ax, 1                   
  01395  03f0               add    si, ax                  
  01397  8b04               mov    ax, word ptr [si]       
  01399  83c005             add    ax, 5                   
  0139c  59                 pop    cx                      
  0139d  99                 cdq                            
  0139e  f7f9               idiv   cx                      
  013a0  f7eb               imul   bx                      
  013a2  be649f             mov    si, 0x9f64              
  013a5  8b5604             mov    dx, word ptr [bp + 4]   
  013a8  d1e2               shl    dx, 1                   
  013aa  03f2               add    si, dx                  
  013ac  8b14               mov    dx, word ptr [si]       
  013ae  83c201             add    dx, 1                   
  013b1  b90300             mov    cx, 3                   
  013b4  d3fa               sar    dx, cl                  
  013b6  03c2               add    ax, dx                  
  013b8  8be5               mov    sp, bp                  
  013ba  5d                 pop    bp                      
  013bb  c3                 ret                            
  013bc  55                 push   bp                      
  013bd  8bec               mov    bp, sp                  
  013bf  83ec08             sub    sp, 8                   
  013c2  bed89e             mov    si, 0x9ed8              
  013c5  8b060aa5           mov    ax, word ptr [0xa50a]   
  013c9  d1e0               shl    ax, 1                   
  013cb  03f0               add    si, ax                  
  013cd  8b04               mov    ax, word ptr [si]       
  013cf  40                 inc    ax                      
  013d0  8946fc             mov    word ptr [bp - 4], ax   
  013d3  be009f             mov    si, 0x9f00              
  013d6  8b060aa5           mov    ax, word ptr [0xa50a]   
  013da  d1e0               shl    ax, 1                   
  013dc  03f0               add    si, ax                  
  013de  8b04               mov    ax, word ptr [si]       
  013e0  8946fe             mov    word ptr [bp - 2], ax   
  013e3  33c0               xor    ax, ax                  
  013e5  8946f8             mov    word ptr [bp - 8], ax   
  013e8  8b46fc             mov    ax, word ptr [bp - 4]   
  013eb  3b4604             cmp    ax, word ptr [bp + 4]   
  013ee  750b               jne    0x13fb                  
  013f0  8b46fe             mov    ax, word ptr [bp - 2]   
  013f3  3b4606             cmp    ax, word ptr [bp + 6]   
  013f6  7503               jne    0x13fb                  
  013f8  e99e00             jmp    0x1499                  
  013fb  8b460a             mov    ax, word ptr [bp + 0xa] 
  013fe  0bc0               or     ax, ax                  
  01400  7503               jne    0x1405                  
  01402  e99400             jmp    0x1499                  
  01405  33c0               xor    ax, ax                  
  01407  8946fa             mov    word ptr [bp - 6], ax   
  0140a  ff46f8             inc    word ptr [bp - 8]       
  0140d  8b46f8             mov    ax, word ptr [bp - 8]   
  01410  83f805             cmp    ax, 5                   
  01413  7d1e               jge    0x1433                  
  01415  8b46fc             mov    ax, word ptr [bp - 4]   
  01418  3b4604             cmp    ax, word ptr [bp + 4]   
  0141b  7d06               jge    0x1423                  
  0141d  ff46fa             inc    word ptr [bp - 6]       
  01420  ff46fc             inc    word ptr [bp - 4]       
  01423  8b46fc             mov    ax, word ptr [bp - 4]   
  01426  3b4604             cmp    ax, word ptr [bp + 4]   
  01429  7e06               jle    0x1431                  
  0142b  ff4efa             dec    word ptr [bp - 6]       
  0142e  ff4efc             dec    word ptr [bp - 4]       
  01431  eb05               jmp    0x1438                  
  01433  33c0               xor    ax, ax                  
  01435  8946f8             mov    word ptr [bp - 8], ax   
  01438  8b46fe             mov    ax, word ptr [bp - 2]   
  0143b  3b4606             cmp    ax, word ptr [bp + 6]   
  0143e  7d08               jge    0x1448                  
  01440  8146fa5000         add    word ptr [bp - 6], 0x50 
  01445  ff46fe             inc    word ptr [bp - 2]       
  01448  8b46fe             mov    ax, word ptr [bp - 2]   
  0144b  3b4606             cmp    ax, word ptr [bp + 6]   
  0144e  7e08               jle    0x1458                  
  01450  816efa5000         sub    word ptr [bp - 6], 0x50 
  01455  ff4efe             dec    word ptr [bp - 2]       
  01458  817efa0000         cmp    word ptr [bp - 6], 0    
  0145d  7489               je     0x13e8                  
  0145f  ff7608             push   word ptr [bp + 8]       
  01462  e804ff             call   0x1369                  
  01465  5f                 pop    di                      
  01466  894608             mov    word ptr [bp + 8], ax   
  01469  be8297             mov    si, 0x9782              
  0146c  037608             add    si, word ptr [bp + 8]   
  0146f  8a04               mov    al, byte ptr [si]       
  01471  98                 cwde                           
  01472  0bc0               or     ax, ax                  
  01474  7412               je     0x1488                  
  01476  ff4e08             dec    word ptr [bp + 8]       
  01479  8b4608             mov    ax, word ptr [bp + 8]   
  0147c  09c0               or     ax, ax                  
  0147e  7d06               jge    0x1486                  
  01480  b8e703             mov    ax, 0x3e7               
  01483  894608             mov    word ptr [bp + 8], ax   
  01486  eb11               jmp    0x1499                  
  01488  8b46fa             mov    ax, word ptr [bp - 6]   
  0148b  be8297             mov    si, 0x9782              
  0148e  037608             add    si, word ptr [bp + 8]   
  01491  8804               mov    byte ptr [si], al       
  01493  ff4e0a             dec    word ptr [bp + 0xa]     
  01496  e94fff             jmp    0x13e8                  
  01499  8b4608             mov    ax, word ptr [bp + 8]   
  0149c  8be5               mov    sp, bp                  
  0149e  5d                 pop    bp                      
  0149f  c3                 ret                            
  014a0  55                 push   bp                      
  014a1  8bec               mov    bp, sp                  
  014a3  83ec0a             sub    sp, 0xa                 
  014a6  33c0               xor    ax, ax                  
  014a8  8946fe             mov    word ptr [bp - 2], ax   
  014ab  8946fc             mov    word ptr [bp - 4], ax   
  014ae  8946f6             mov    word ptr [bp - 0xa], ax 
  014b1  8946fa             mov    word ptr [bp - 6], ax   
  014b4  8946f8             mov    word ptr [bp - 8], ax   
  014b7  b80100             mov    ax, 1                   
  014ba  0bc0               or     ax, ax                  
  014bc  745e               je     0x151c                  
  014be  be6a9b             mov    si, 0x9b6a              
  014c1  0376fc             add    si, word ptr [bp - 4]   
  014c4  8a04               mov    al, byte ptr [si]       
  014c6  98                 cwde                           
  014c7  09c0               or     ax, ax                  
  014c9  750f               jne    0x14da                  
  014cb  817efcc800         cmp    word ptr [bp - 4], 0xc8 
  014d0  7d08               jge    0x14da                  
  014d2  ff46f8             inc    word ptr [bp - 8]       
  014d5  ff46fc             inc    word ptr [bp - 4]       
  014d8  ebe4               jmp    0x14be                  
  014da  8b46f8             mov    ax, word ptr [bp - 8]   
  014dd  3b46fa             cmp    ax, word ptr [bp - 6]   
  014e0  7e0c               jle    0x14ee                  
  014e2  8b46f8             mov    ax, word ptr [bp - 8]   
  014e5  8946fa             mov    word ptr [bp - 6], ax   
  014e8  8b46fe             mov    ax, word ptr [bp - 2]   
  014eb  8946f6             mov    word ptr [bp - 0xa], ax 
  014ee  33c0               xor    ax, ax                  
  014f0  8946f8             mov    word ptr [bp - 8], ax   
  014f3  be6a9b             mov    si, 0x9b6a              
  014f6  0376fc             add    si, word ptr [bp - 4]   
  014f9  8a04               mov    al, byte ptr [si]       
  014fb  98                 cwde                           
  014fc  0bc0               or     ax, ax                  
  014fe  740d               je     0x150d                  
  01500  ff46fc             inc    word ptr [bp - 4]       
  01503  8b46fc             mov    ax, word ptr [bp - 4]   
  01506  3dc700             cmp    ax, 0xc7                
  01509  7d02               jge    0x150d                  
  0150b  ebe6               jmp    0x14f3                  
  0150d  8b46fc             mov    ax, word ptr [bp - 4]   
  01510  8946fe             mov    word ptr [bp - 2], ax   
  01513  817efcc700         cmp    word ptr [bp - 4], 0xc7 
  01518  7d02               jge    0x151c                  
  0151a  eb9b               jmp    0x14b7                  
  0151c  8b46f6             mov    ax, word ptr [bp - 0xa] 
  0151f  8be5               mov    sp, bp                  
  01521  5d                 pop    bp                      
  01522  c3                 ret                            
  01523  55                 push   bp                      
  01524  8bec               mov    bp, sp                  
  01526  83ec06             sub    sp, 6                   
  01529  bef4a0             mov    si, 0xa0f4              
  0152c  8b4604             mov    ax, word ptr [bp + 4]   
  0152f  d1e0               shl    ax, 1                   
  01531  03f0               add    si, ax                  
  01533  8b04               mov    ax, word ptr [si]       
  01535  0bc0               or     ax, ax                  
  01537  750e               jne    0x1547                  
  01539  bef09f             mov    si, 0x9ff0              
  0153c  037604             add    si, word ptr [bp + 4]   
  0153f  8a04               mov    al, byte ptr [si]       
  01541  98                 cwde                           
  01542  83f80e             cmp    ax, 0xe                 
  01545  7c06               jl     0x154d                  
  01547  33c0               xor    ax, ax                  
  01549  8be5               mov    sp, bp                  
  0154b  5d                 pop    bp                      
  0154c  c3                 ret                            
  0154d  33c0               xor    ax, ax                  
  0154f  8946fa             mov    word ptr [bp - 6], ax   
  01552  817efa1400         cmp    word ptr [bp - 6], 0x14 
  01557  7c03               jl     0x155c                  
  01559  e9b900             jmp    0x1615                  
  0155c  beb09e             mov    si, 0x9eb0              
  0155f  8b46fa             mov    ax, word ptr [bp - 6]   
  01562  d1e0               shl    ax, 1                   
  01564  03f0               add    si, ax                  
  01566  8b04               mov    ax, word ptr [si]       
  01568  3d0f27             cmp    ax, 0x270f              
  0156b  741a               je     0x1587                  
  0156d  be609e             mov    si, 0x9e60              
  01570  8b46fa             mov    ax, word ptr [bp - 6]   
  01573  d1e0               shl    ax, 1                   
  01575  03f0               add    si, ax                  
  01577  bf609e             mov    di, 0x9e60              
  0157a  8b4604             mov    ax, word ptr [bp + 4]   
  0157d  d1e0               shl    ax, 1                   
  0157f  03f8               add    di, ax                  
  01581  8b05               mov    ax, word ptr [di]       
  01583  3b04               cmp    ax, word ptr [si]       
  01585  7402               je     0x1589                  
  01587  eb08               jmp    0x1591                  
  01589  8b46fa             mov    ax, word ptr [bp - 6]   
  0158c  3b4604             cmp    ax, word ptr [bp + 4]   
  0158f  7502               jne    0x1593                  
  01591  eb10               jmp    0x15a3                  
  01593  bef4a0             mov    si, 0xa0f4              
  01596  8b46fa             mov    ax, word ptr [bp - 6]   
  01599  d1e0               shl    ax, 1                   
  0159b  03f0               add    si, ax                  
  0159d  8b04               mov    ax, word ptr [si]       
  0159f  0bc0               or     ax, ax                  
  015a1  7402               je     0x15a5                  
  015a3  eb6a               jmp    0x160f                  
  015a5  bed89e             mov    si, 0x9ed8              
  015a8  8b46fa             mov    ax, word ptr [bp - 6]   
  015ab  d1e0               shl    ax, 1                   
  015ad  03f0               add    si, ax                  
  015af  8b04               mov    ax, word ptr [si]       
  015b1  bed89e             mov    si, 0x9ed8              
  015b4  8b5604             mov    dx, word ptr [bp + 4]   
  015b7  d1e2               shl    dx, 1                   
  015b9  03f2               add    si, dx                  
  015bb  2b04               sub    ax, word ptr [si]       
  015bd  8946fc             mov    word ptr [bp - 4], ax   
  015c0  be009f             mov    si, 0x9f00              
  015c3  8b46fa             mov    ax, word ptr [bp - 6]   
  015c6  d1e0               shl    ax, 1                   
  015c8  03f0               add    si, ax                  
  015ca  8b04               mov    ax, word ptr [si]       
  015cc  be009f             mov    si, 0x9f00              
  015cf  8b5604             mov    dx, word ptr [bp + 4]   
  015d2  d1e2               shl    dx, 1                   
  015d4  03f2               add    si, dx                  
  015d6  2b04               sub    ax, word ptr [si]       
  015d8  8946fe             mov    word ptr [bp - 2], ax   
  015db  817efc0000         cmp    word ptr [bp - 4], 0    
  015e0  7d08               jge    0x15ea                  
  015e2  8b46fc             mov    ax, word ptr [bp - 4]   
  015e5  f7d8               neg    ax                      
  015e7  8946fc             mov    word ptr [bp - 4], ax   
  015ea  817efe0000         cmp    word ptr [bp - 2], 0    
  015ef  7d08               jge    0x15f9                  
  015f1  8b46fe             mov    ax, word ptr [bp - 2]   
  015f4  f7d8               neg    ax                      
  015f6  8946fe             mov    word ptr [bp - 2], ax   
  015f9  817efc0300         cmp    word ptr [bp - 4], 3    
  015fe  7d0f               jge    0x160f                  
  01600  817efe0700         cmp    word ptr [bp - 2], 7    
  01605  7d08               jge    0x160f                  
  01607  8b46fa             mov    ax, word ptr [bp - 6]   
  0160a  40                 inc    ax                      
  0160b  8be5               mov    sp, bp                  
  0160d  5d                 pop    bp                      
  0160e  c3                 ret                            
  0160f  ff46fa             inc    word ptr [bp - 6]       
  01612  e93dff             jmp    0x1552                  
  01615  33c0               xor    ax, ax                  
  01617  8be5               mov    sp, bp                  
  01619  5d                 pop    bp                      
  0161a  c3                 ret                            
  0161b  55                 push   bp                      
  0161c  8bec               mov    bp, sp                  
  0161e  33c0               xor    ax, ax                  
  01620  bedc9f             mov    si, 0x9fdc              
  01623  037606             add    si, word ptr [bp + 6]   
  01626  8804               mov    byte ptr [si], al       
  01628  98                 cwde                           
  01629  bea09f             mov    si, 0x9fa0              
  0162c  037606             add    si, word ptr [bp + 6]   
  0162f  8804               mov    byte ptr [si], al       
  01631  98                 cwde                           
  01632  bec89f             mov    si, 0x9fc8              
  01635  037606             add    si, word ptr [bp + 6]   
  01638  8804               mov    byte ptr [si], al       
  0163a  98                 cwde                           
  0163b  beb49f             mov    si, 0x9fb4              
  0163e  037606             add    si, word ptr [bp + 6]   
  01641  8804               mov    byte ptr [si], al       
  01643  33c0               xor    ax, ax                  
  01645  bebca1             mov    si, 0xa1bc              
  01648  8b5606             mov    dx, word ptr [bp + 6]   
  0164b  d1e2               shl    dx, 1                   
  0164d  03f2               add    si, dx                  
  0164f  8904               mov    word ptr [si], ax       
  01651  be94a1             mov    si, 0xa194              
  01654  8b5606             mov    dx, word ptr [bp + 6]   
  01657  d1e2               shl    dx, 1                   
  01659  03f2               add    si, dx                  
  0165b  8904               mov    word ptr [si], ax       
  0165d  beb8a0             mov    si, 0xa0b8              
  01660  037606             add    si, word ptr [bp + 6]   
  01663  8804               mov    byte ptr [si], al       
  01665  b80200             mov    ax, 2                   
  01668  be889e             mov    si, 0x9e88              
  0166b  8b5606             mov    dx, word ptr [bp + 6]   
  0166e  d1e2               shl    dx, 1                   
  01670  03f2               add    si, dx                  
  01672  8904               mov    word ptr [si], ax       
  01674  bed89e             mov    si, 0x9ed8              
  01677  8b4604             mov    ax, word ptr [bp + 4]   
  0167a  d1e0               shl    ax, 1                   
  0167c  03f0               add    si, ax                  
  0167e  8b04               mov    ax, word ptr [si]       
  01680  bed89e             mov    si, 0x9ed8              
  01683  8b5606             mov    dx, word ptr [bp + 6]   
  01686  d1e2               shl    dx, 1                   
  01688  03f2               add    si, dx                  
  0168a  8904               mov    word ptr [si], ax       
  0168c  be009f             mov    si, 0x9f00              
  0168f  8b4604             mov    ax, word ptr [bp + 4]   
  01692  d1e0               shl    ax, 1                   
  01694  03f0               add    si, ax                  
  01696  8b04               mov    ax, word ptr [si]       
  01698  be009f             mov    si, 0x9f00              
  0169b  8b5606             mov    dx, word ptr [bp + 6]   
  0169e  d1e2               shl    dx, 1                   
  016a0  03f2               add    si, dx                  
  016a2  8904               mov    word ptr [si], ax       
  016a4  be68a0             mov    si, 0xa068              
  016a7  8b4604             mov    ax, word ptr [bp + 4]   
  016aa  d1e0               shl    ax, 1                   
  016ac  03f0               add    si, ax                  
  016ae  8b04               mov    ax, word ptr [si]       
  016b0  be68a0             mov    si, 0xa068              
  016b3  8b5606             mov    dx, word ptr [bp + 6]   
  016b6  d1e2               shl    dx, 1                   
  016b8  03f2               add    si, dx                  
  016ba  8904               mov    word ptr [si], ax       
  016bc  be609e             mov    si, 0x9e60              
  016bf  8b4604             mov    ax, word ptr [bp + 4]   
  016c2  d1e0               shl    ax, 1                   
  016c4  03f0               add    si, ax                  
  016c6  8b04               mov    ax, word ptr [si]       
  016c8  83f8ff             cmp    ax, -1                  
  016cb  7518               jne    0x16e5                  
  016cd  bed89e             mov    si, 0x9ed8              
  016d0  8b4606             mov    ax, word ptr [bp + 6]   
  016d3  d1e0               shl    ax, 1                   
  016d5  03f0               add    si, ax                  
  016d7  ff04               inc    word ptr [si]           
  016d9  be68a0             mov    si, 0xa068              
  016dc  8b4606             mov    ax, word ptr [bp + 6]   
  016df  d1e0               shl    ax, 1                   
  016e1  03f0               add    si, ax                  
  016e3  ff04               inc    word ptr [si]           
  016e5  bed89e             mov    si, 0x9ed8              
  016e8  8b4606             mov    ax, word ptr [bp + 6]   
  016eb  d1e0               shl    ax, 1                   
  016ed  03f0               add    si, ax                  
  016ef  8b04               mov    ax, word ptr [si]       
  016f1  83f846             cmp    ax, 0x46                
  016f4  7e26               jle    0x171c                  
  016f6  b80200             mov    ax, 2                   
  016f9  bed89e             mov    si, 0x9ed8              
  016fc  8b5606             mov    dx, word ptr [bp + 6]   
  016ff  d1e2               shl    dx, 1                   
  01701  03f2               add    si, dx                  
  01703  8b14               mov    dx, word ptr [si]       
  01705  2bd0               sub    dx, ax                  
  01707  8914               mov    word ptr [si], dx       
  01709  b80200             mov    ax, 2                   
  0170c  be68a0             mov    si, 0xa068              
  0170f  8b5606             mov    dx, word ptr [bp + 6]   
  01712  d1e2               shl    dx, 1                   
  01714  03f2               add    si, dx                  
  01716  8b14               mov    dx, word ptr [si]       
  01718  2bd0               sub    dx, ax                  
  0171a  8914               mov    word ptr [si], dx       
  0171c  be3c9f             mov    si, 0x9f3c              
  0171f  037604             add    si, word ptr [bp + 4]   
  01722  8a04               mov    al, byte ptr [si]       
  01724  98                 cwde                           
  01725  be3c9f             mov    si, 0x9f3c              
  01728  037606             add    si, word ptr [bp + 6]   
  0172b  8804               mov    byte ptr [si], al       
  0172d  be509f             mov    si, 0x9f50              
  01730  037604             add    si, word ptr [bp + 4]   
  01733  8a04               mov    al, byte ptr [si]       
  01735  98                 cwde                           
  01736  be509f             mov    si, 0x9f50              
  01739  037606             add    si, word ptr [bp + 6]   
  0173c  8804               mov    byte ptr [si], al       
  0173e  be289f             mov    si, 0x9f28              
  01741  037604             add    si, word ptr [bp + 4]   
  01744  8a04               mov    al, byte ptr [si]       
  01746  98                 cwde                           
  01747  be289f             mov    si, 0x9f28              
  0174a  037606             add    si, word ptr [bp + 6]   
  0174d  8804               mov    byte ptr [si], al       
  0174f  be609e             mov    si, 0x9e60              
  01752  8b4604             mov    ax, word ptr [bp + 4]   
  01755  d1e0               shl    ax, 1                   
  01757  03f0               add    si, ax                  
  01759  8b04               mov    ax, word ptr [si]       
  0175b  be609e             mov    si, 0x9e60              
  0175e  8b5606             mov    dx, word ptr [bp + 6]   
  01761  d1e2               shl    dx, 1                   
  01763  03f2               add    si, dx                  
  01765  8904               mov    word ptr [si], ax       
  01767  8be5               mov    sp, bp                  
  01769  5d                 pop    bp                      
  0176a  c3                 ret                            
  0176b  55                 push   bp                      
  0176c  8bec               mov    bp, sp                  
  0176e  817e040a00         cmp    word ptr [bp + 4], 0xa  
  01773  7c25               jl     0x179a                  
  01775  b80a00             mov    ax, 0xa                 
  01778  50                 push   ax                      
  01779  8b4604             mov    ax, word ptr [bp + 4]   
  0177c  5b                 pop    bx                      
  0177d  99                 cdq                            
  0177e  f7fb               idiv   bx                      
  01780  50                 push   ax                      
  01781  e8e7ff             call   0x176b                  
  01784  5f                 pop    di                      
  01785  b80a00             mov    ax, 0xa                 
  01788  50                 push   ax                      
  01789  8b4604             mov    ax, word ptr [bp + 4]   
  0178c  5b                 pop    bx                      
  0178d  99                 cdq                            
  0178e  f7fb               idiv   bx                      
  01790  83c230             add    dx, 0x30                
  01793  52                 push   dx                      
  01794  e8500d             call   0x24e7                  
  01797  5f                 pop    di                      
  01798  eb0b               jmp    0x17a5                  
  0179a  8b4604             mov    ax, word ptr [bp + 4]   
  0179d  83c030             add    ax, 0x30                
  017a0  50                 push   ax                      
  017a1  e8430d             call   0x24e7                  
  017a4  5f                 pop    di                      
  017a5  8be5               mov    sp, bp                  
  017a7  5d                 pop    bp                      
  017a8  c3                 ret                            
  017a9  06                 push   es                      
  017aa  55                 push   bp                      
  017ab  8bec               mov    bp, sp                  
  017ad  fc                 cld                            
  017ae  8b366888           mov    si, word ptr [0x8868]   
  017b2  8bd6               mov    dx, si                  
  017b4  81c2803e           add    dx, 0x3e80              
  017b8  a15909             mov    ax, word ptr [0x959]    
  017bb  8ec0               mov    es, ax                  
  017bd  b82800             mov    ax, 0x28                
  017c0  bb0000             mov    bx, 0                   
  017c3  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  017c7  8bc8               mov    cx, ax                  
  017c9  f3a5               rep movsw word ptr es:[di], word ptr [si]
  017cb  87d6               xchg   si, dx                  
  017cd  43                 inc    bx                      
  017ce  43                 inc    bx                      
  017cf  81fbc800           cmp    bx, 0xc8                
  017d3  7cee               jl     0x17c3                  
  017d5  8b6e06             mov    bp, word ptr [bp + 6]   
  017d8  03eb               add    bp, bx                  
  017da  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  017de  8bc8               mov    cx, ax                  
  017e0  f3a5               rep movsw word ptr es:[di], word ptr [si]
  017e2  87d6               xchg   si, dx                  
  017e4  43                 inc    bx                      
  017e5  43                 inc    bx                      
  017e6  3bdd               cmp    bx, bp                  
  017e8  7cf0               jl     0x17da                  
  017ea  5d                 pop    bp                      
  017eb  07                 pop    es                      
  017ec  c3                 ret                            
  017ed  55                 push   bp                      
  017ee  8bec               mov    bp, sp                  
  017f0  fc                 cld                            
  017f1  b80400             mov    ax, 4                   
  017f4  50                 push   ax                      
  017f5  8b4604             mov    ax, word ptr [bp + 4]   
  017f8  50                 push   ax                      
  017f9  e80e0e             call   0x260a                  
  017fc  5b                 pop    bx                      
  017fd  5b                 pop    bx                      
  017fe  3d0f27             cmp    ax, 0x270f              
  01801  740d               je     0x1810                  
  01803  3d0000             cmp    ax, 0                   
  01806  750c               jne    0x1814                  
  01808  e8f5eb             call   0x400                   
  0180b  3d0000             cmp    ax, 0                   
  0180e  74e1               je     0x17f1                  
  01810  33c0               xor    ax, ax                  
  01812  eb20               jmp    0x1834                  
  01814  a3aaa9             mov    word ptr [0xa9aa], ax   
  01817  8b4608             mov    ax, word ptr [bp + 8]   
  0181a  50                 push   ax                      
  0181b  8b4606             mov    ax, word ptr [bp + 6]   
  0181e  50                 push   ax                      
  0181f  a1aaa9             mov    ax, word ptr [0xa9aa]   
  01822  50                 push   ax                      
  01823  e8280e             call   0x264e                  
  01826  5a                 pop    dx                      
  01827  5a                 pop    dx                      
  01828  5a                 pop    dx                      
  01829  a1aaa9             mov    ax, word ptr [0xa9aa]   
  0182c  50                 push   ax                      
  0182d  e85a0e             call   0x268a                  
  01830  58                 pop    ax                      
  01831  b80100             mov    ax, 1                   
  01834  5d                 pop    bp                      
  01835  c3                 ret                            
  01836  8d36549e           lea    si, [0x9e54]            
  0183a  8b1e4e9e           mov    bx, word ptr [0x9e4e]   
  0183e  833e2fa902         cmp    word ptr [0xa92f], 2    
  01843  7436               je     0x187b                  
  01845  e82600             call   0x186e                  
  01848  8a4750             mov    al, byte ptr [bx + 0x50]
  0184b  8804               mov    byte ptr [si], al       
  0184d  46                 inc    si                      
  0184e  81c39f00           add    bx, 0x9f                
  01852  e81900             call   0x186e                  
  01855  43                 inc    bx                      
  01856  e81500             call   0x186e                  
  01859  43                 inc    bx                      
  0185a  e81100             call   0x186e                  
  0185d  81c39f00           add    bx, 0x9f                
  01861  8a87303e           mov    al, byte ptr [bx + 0x3e30]
  01865  8804               mov    byte ptr [si], al       
  01867  46                 inc    si                      
  01868  e80300             call   0x186e                  
  0186b  eb3b               jmp    0x18a8                  
  0186d  90                 nop                            
  0186e  8a07               mov    al, byte ptr [bx]       
  01870  8804               mov    byte ptr [si], al       
  01872  46                 inc    si                      
  01873  8a87803e           mov    al, byte ptr [bx + 0x3e80]
  01877  8804               mov    byte ptr [si], al       
  01879  46                 inc    si                      
  0187a  c3                 ret                            
  0187b  8b07               mov    ax, word ptr [bx]       
  0187d  8904               mov    word ptr [si], ax       
  0187f  46                 inc    si                      
  01880  46                 inc    si                      
  01881  8b4750             mov    ax, word ptr [bx + 0x50]
  01884  8904               mov    word ptr [si], ax       
  01886  46                 inc    si                      
  01887  46                 inc    si                      
  01888  8b87a000           mov    ax, word ptr [bx + 0xa0]
  0188c  8904               mov    word ptr [si], ax       
  0188e  46                 inc    si                      
  0188f  46                 inc    si                      
  01890  8b874001           mov    ax, word ptr [bx + 0x140]
  01894  8904               mov    word ptr [si], ax       
  01896  46                 inc    si                      
  01897  46                 inc    si                      
  01898  8b879001           mov    ax, word ptr [bx + 0x190]
  0189c  8904               mov    word ptr [si], ax       
  0189e  46                 inc    si                      
  0189f  46                 inc    si                      
  018a0  8b87e001           mov    ax, word ptr [bx + 0x1e0]
  018a4  8904               mov    word ptr [si], ax       
  018a6  46                 inc    si                      
  018a7  46                 inc    si                      
  018a8  c3                 ret                            
  018a9  8b1e4e9e           mov    bx, word ptr [0x9e4e]   
  018ad  be803e             mov    si, 0x3e80              
  018b0  833e2fa902         cmp    word ptr [0xa92f], 2    
  018b5  7439               je     0x18f0                  
  018b7  b00f               mov    al, 0xf                 
  018b9  8807               mov    byte ptr [bx], al       
  018bb  8800               mov    byte ptr [bx + si], al  
  018bd  884750             mov    byte ptr [bx + 0x50], al
  018c0  81c39f00           add    bx, 0x9f                
  018c4  8807               mov    byte ptr [bx], al       
  018c6  8800               mov    byte ptr [bx + si], al  
  018c8  43                 inc    bx                      
  018c9  8a07               mov    al, byte ptr [bx]       
  018cb  0cc0               or     al, 0xc0                
  018cd  24fc               and    al, 0xfc                
  018cf  8807               mov    byte ptr [bx], al       
  018d1  8a00               mov    al, byte ptr [bx + si]  
  018d3  0cc0               or     al, 0xc0                
  018d5  24fc               and    al, 0xfc                
  018d7  8800               mov    byte ptr [bx + si], al  
  018d9  43                 inc    bx                      
  018da  b03f               mov    al, 0x3f                
  018dc  8807               mov    byte ptr [bx], al       
  018de  8800               mov    byte ptr [bx + si], al  
  018e0  81c39f00           add    bx, 0x9f                
  018e4  b00f               mov    al, 0xf                 
  018e6  8887303e           mov    byte ptr [bx + 0x3e30], al
  018ea  8807               mov    byte ptr [bx], al       
  018ec  8800               mov    byte ptr [bx + si], al  
  018ee  eb1b               jmp    0x190b                  
  018f0  b80ff0             mov    ax, 0xf00f              
  018f3  8907               mov    word ptr [bx], ax       
  018f5  8987e001           mov    word ptr [bx + 0x1e0], ax
  018f9  b8f00f             mov    ax, 0xff0               
  018fc  094750             or     word ptr [bx + 0x50], ax
  018ff  0987a000           or     word ptr [bx + 0xa0], ax
  01903  09874001           or     word ptr [bx + 0x140], ax
  01907  09879001           or     word ptr [bx + 0x190], ax
  0190b  c3                 ret                            
  0190c  fc                 cld                            
  0190d  8d36549e           lea    si, [0x9e54]            
  01911  8b1e4e9e           mov    bx, word ptr [0x9e4e]   
  01915  833e2fa902         cmp    word ptr [0xa92f], 2    
  0191a  742e               je     0x194a                  
  0191c  e82200             call   0x1941                  
  0191f  ac                 lodsb  al, byte ptr [si]       
  01920  884750             mov    byte ptr [bx + 0x50], al
  01923  81c39f00           add    bx, 0x9f                
  01927  e81700             call   0x1941                  
  0192a  43                 inc    bx                      
  0192b  e81300             call   0x1941                  
  0192e  43                 inc    bx                      
  0192f  e80f00             call   0x1941                  
  01932  81c39f00           add    bx, 0x9f                
  01936  ac                 lodsb  al, byte ptr [si]       
  01937  8887303e           mov    byte ptr [bx + 0x3e30], al
  0193b  e80300             call   0x1941                  
  0193e  eb25               jmp    0x1965                  
  01940  90                 nop                            
  01941  ac                 lodsb  al, byte ptr [si]       
  01942  8807               mov    byte ptr [bx], al       
  01944  ac                 lodsb  al, byte ptr [si]       
  01945  8887803e           mov    byte ptr [bx + 0x3e80], al
  01949  c3                 ret                            
  0194a  ad                 lodsw  ax, word ptr [si]       
  0194b  8907               mov    word ptr [bx], ax       
  0194d  ad                 lodsw  ax, word ptr [si]       
  0194e  894750             mov    word ptr [bx + 0x50], ax
  01951  ad                 lodsw  ax, word ptr [si]       
  01952  8987a000           mov    word ptr [bx + 0xa0], ax
  01956  ad                 lodsw  ax, word ptr [si]       
  01957  89874001           mov    word ptr [bx + 0x140], ax
  0195b  ad                 lodsw  ax, word ptr [si]       
  0195c  89879001           mov    word ptr [bx + 0x190], ax
  01960  ad                 lodsw  ax, word ptr [si]       
  01961  8987e001           mov    word ptr [bx + 0x1e0], ax
  01965  c3                 ret                            
  01966  06                 push   es                      
  01967  fc                 cld                            
  01968  8cd8               mov    ax, ds                  
  0196a  8ec0               mov    es, ax                  
  0196c  b84e00             mov    ax, 0x4e                
  0196f  bb0000             mov    bx, 0                   
  01972  8d3e0ca2           lea    di, [0xa20c]            
  01976  8bb768a0           mov    si, word ptr [bx - 0x5f98]
  0197a  8bd6               mov    dx, si                  
  0197c  81c2803e           add    dx, 0x3e80              
  01980  b90600             mov    cx, 6                   
  01983  a5                 movsw  word ptr es:[di], word ptr [si]
  01984  87d6               xchg   si, dx                  
  01986  a5                 movsw  word ptr es:[di], word ptr [si]
  01987  87d6               xchg   si, dx                  
  01989  03f0               add    si, ax                  
  0198b  03d0               add    dx, ax                  
  0198d  e2f4               loop   0x1983                  
  0198f  83c302             add    bx, 2                   
  01992  83fb28             cmp    bx, 0x28                
  01995  75df               jne    0x1976                  
  01997  07                 pop    es                      
  01998  c3                 ret                            
  01999  06                 push   es                      
  0199a  8cd8               mov    ax, ds                  
  0199c  8ec0               mov    es, ax                  
  0199e  fc                 cld                            
  0199f  b84e00             mov    ax, 0x4e                
  019a2  bb0000             mov    bx, 0                   
  019a5  8d360ca2           lea    si, [0xa20c]            
  019a9  81bfb09e0f27       cmp    word ptr [bx - 0x6150], 0x270f
  019af  7505               jne    0x19b6                  
  019b1  83c618             add    si, 0x18                
  019b4  eb19               jmp    0x19cf                  
  019b6  8bbf68a0           mov    di, word ptr [bx - 0x5f98]
  019ba  8bd7               mov    dx, di                  
  019bc  81c2803e           add    dx, 0x3e80              
  019c0  b90600             mov    cx, 6                   
  019c3  a5                 movsw  word ptr es:[di], word ptr [si]
  019c4  87d7               xchg   di, dx                  
  019c6  a5                 movsw  word ptr es:[di], word ptr [si]
  019c7  87d7               xchg   di, dx                  
  019c9  03f8               add    di, ax                  
  019cb  03d0               add    dx, ax                  
  019cd  e2f4               loop   0x19c3                  
  019cf  83c302             add    bx, 2                   
  019d2  83fb28             cmp    bx, 0x28                
  019d5  75d2               jne    0x19a9                  
  019d7  07                 pop    es                      
  019d8  c3                 ret                            
  019d9  06                 push   es                      
  019da  55                 push   bp                      
  019db  fc                 cld                            
  019dc  8cd8               mov    ax, ds                  
  019de  8ec0               mov    es, ax                  
  019e0  bb0000             mov    bx, 0                   
  019e3  81bfb09e0f27       cmp    word ptr [bx - 0x6150], 0x270f
  019e9  53                 push   bx                      
  019ea  746c               je     0x1a58                  
  019ec  81bf609effff       cmp    word ptr [bx - 0x61a0], 0xffff
  019f2  740e               je     0x1a02                  
  019f4  833e0fa750         cmp    word ptr [0xa70f], 0x50 
  019f9  7407               je     0x1a02                  
  019fb  83bf1ca100         cmp    word ptr [bx - 0x5ee4], 0
  01a00  7456               je     0x1a58                  
  01a02  8b87609e           mov    ax, word ptr [bx - 0x61a0]
  01a06  50                 push   ax                      
  01a07  8b8768a0           mov    ax, word ptr [bx - 0x5f98]
  01a0b  50                 push   ax                      
  01a0c  83bf90a001         cmp    word ptr [bx - 0x5f70], 1
  01a11  753a               jne    0x1a4d                  
  01a13  83bff4a000         cmp    word ptr [bx - 0x5f0c], 0
  01a18  7406               je     0x1a20                  
  01a1a  8d065403           lea    ax, [0x354]             
  01a1e  eb31               jmp    0x1a51                  
  01a20  83bfb09e00         cmp    word ptr [bx - 0x6150], 0
  01a25  750d               jne    0x1a34                  
  01a27  83bfbca100         cmp    word ptr [bx - 0x5e44], 0
  01a2c  7506               jne    0x1a34                  
  01a2e  8d063803           lea    ax, [0x338]             
  01a32  eb1d               jmp    0x1a51                  
  01a34  8bc3               mov    ax, bx                  
  01a36  d1eb               shr    bx, 1                   
  01a38  80bf3c9f0a         cmp    byte ptr [bx - 0x60c4], 0xa
  01a3d  8bd8               mov    bx, ax                  
  01a3f  7706               ja     0x1a47                  
  01a41  8d067003           lea    ax, [0x370]             
  01a45  eb0a               jmp    0x1a51                  
  01a47  8d061c03           lea    ax, [0x31c]             
  01a4b  eb04               jmp    0x1a51                  
  01a4d  8d068c03           lea    ax, [0x38c]             
  01a51  50                 push   ax                      
  01a52  e8c200             call   0x1b17                  
  01a55  58                 pop    ax                      
  01a56  58                 pop    ax                      
  01a57  58                 pop    ax                      
  01a58  5b                 pop    bx                      
  01a59  83c302             add    bx, 2                   
  01a5c  83fb28             cmp    bx, 0x28                
  01a5f  7402               je     0x1a63                  
  01a61  eb80               jmp    0x19e3                  
  01a63  5d                 pop    bp                      
  01a64  07                 pop    es                      
  01a65  c3                 ret                            
  01a66  06                 push   es                      
  01a67  fc                 cld                            
  01a68  8cd8               mov    ax, ds                  
  01a6a  8ec0               mov    es, ax                  
  01a6c  b84e00             mov    ax, 0x4e                
  01a6f  bb0000             mov    bx, 0                   
  01a72  8d3e0ca2           lea    di, [0xa20c]            
  01a76  81c7e001           add    di, 0x1e0               
  01a7a  8bb754a0           mov    si, word ptr [bx - 0x5fac]
  01a7e  8bd6               mov    dx, si                  
  01a80  81c2803e           add    dx, 0x3e80              
  01a84  b90600             mov    cx, 6                   
  01a87  a5                 movsw  word ptr es:[di], word ptr [si]
  01a88  87d6               xchg   si, dx                  
  01a8a  a5                 movsw  word ptr es:[di], word ptr [si]
  01a8b  87d6               xchg   si, dx                  
  01a8d  03f0               add    si, ax                  
  01a8f  03d0               add    dx, ax                  
  01a91  e2f4               loop   0x1a87                  
  01a93  83c302             add    bx, 2                   
  01a96  83fb14             cmp    bx, 0x14                
  01a99  75df               jne    0x1a7a                  
  01a9b  07                 pop    es                      
  01a9c  c3                 ret                            
  01a9d  06                 push   es                      
  01a9e  8cd8               mov    ax, ds                  
  01aa0  8ec0               mov    es, ax                  
  01aa2  fc                 cld                            
  01aa3  b84e00             mov    ax, 0x4e                
  01aa6  bb0000             mov    bx, 0                   
  01aa9  8d360ca2           lea    si, [0xa20c]            
  01aad  81c6e001           add    si, 0x1e0               
  01ab1  83bf649fff         cmp    word ptr [bx - 0x609c], -1
  01ab6  7505               jne    0x1abd                  
  01ab8  83c618             add    si, 0x18                
  01abb  eb19               jmp    0x1ad6                  
  01abd  8bbf54a0           mov    di, word ptr [bx - 0x5fac]
  01ac1  8bd7               mov    dx, di                  
  01ac3  81c2803e           add    dx, 0x3e80              
  01ac7  b90600             mov    cx, 6                   
  01aca  a5                 movsw  word ptr es:[di], word ptr [si]
  01acb  87d7               xchg   di, dx                  
  01acd  a5                 movsw  word ptr es:[di], word ptr [si]
  01ace  87d7               xchg   di, dx                  
  01ad0  03f8               add    di, ax                  
  01ad2  03d0               add    dx, ax                  
  01ad4  e2f4               loop   0x1aca                  
  01ad6  83c302             add    bx, 2                   
  01ad9  83fb14             cmp    bx, 0x14                
  01adc  75d3               jne    0x1ab1                  
  01ade  07                 pop    es                      
  01adf  c3                 ret                            
  01ae0  06                 push   es                      
  01ae1  55                 push   bp                      
  01ae2  fc                 cld                            
  01ae3  8cd8               mov    ax, ds                  
  01ae5  8ec0               mov    es, ax                  
  01ae7  bb0000             mov    bx, 0                   
  01aea  83bf649fff         cmp    word ptr [bx - 0x609c], -1
  01aef  53                 push   bx                      
  01af0  7502               jne    0x1af4                  
  01af2  eb15               jmp    0x1b09                  
  01af4  8b878c9f           mov    ax, word ptr [bx - 0x6074]
  01af8  50                 push   ax                      
  01af9  8b8754a0           mov    ax, word ptr [bx - 0x5fac]
  01afd  50                 push   ax                      
  01afe  8d06a803           lea    ax, [0x3a8]             
  01b02  50                 push   ax                      
  01b03  e81100             call   0x1b17                  
  01b06  58                 pop    ax                      
  01b07  58                 pop    ax                      
  01b08  58                 pop    ax                      
  01b09  5b                 pop    bx                      
  01b0a  83c302             add    bx, 2                   
  01b0d  83fb14             cmp    bx, 0x14                
  01b10  7402               je     0x1b14                  
  01b12  ebd6               jmp    0x1aea                  
  01b14  5d                 pop    bp                      
  01b15  07                 pop    es                      
  01b16  c3                 ret                            
  01b17  56                 push   si                      
  01b18  57                 push   di                      
  01b19  55                 push   bp                      
  01b1a  8bec               mov    bp, sp                  
  01b1c  8b7608             mov    si, word ptr [bp + 8]   
  01b1f  8b7e0a             mov    di, word ptr [bp + 0xa] 
  01b22  893e1305           mov    word ptr [0x513], di    
  01b26  8b460c             mov    ax, word ptr [bp + 0xc] 
  01b29  8be8               mov    bp, ax                  
  01b2b  06                 push   es                      
  01b2c  8cd8               mov    ax, ds                  
  01b2e  8ec0               mov    es, ax                  
  01b30  fc                 cld                            
  01b31  ad                 lodsw  ax, word ptr [si]       
  01b32  a31505             mov    word ptr [0x515], ax    
  01b35  ad                 lodsw  ax, word ptr [si]       
  01b36  a31905             mov    word ptr [0x519], ax    
  01b39  83fd00             cmp    bp, 0                   
  01b3c  7503               jne    0x1b41                  
  01b3e  e97f00             jmp    0x1bc0                  
  01b41  8b3e1305           mov    di, word ptr [0x513]    
  01b45  8b0e1505           mov    cx, word ptr [0x515]    
  01b49  ad                 lodsw  ax, word ptr [si]       
  01b4a  86c4               xchg   ah, al                  
  01b4c  8bd0               mov    dx, ax                  
  01b4e  8b1d               mov    bx, word ptr [di]       
  01b50  86df               xchg   bh, bl                  
  01b52  d1e8               shr    ax, 1                   
  01b54  d1e8               shr    ax, 1                   
  01b56  f7d0               not    ax                      
  01b58  23d8               and    bx, ax                  
  01b5a  8bc2               mov    ax, dx                  
  01b5c  d1e0               shl    ax, 1                   
  01b5e  d1e0               shl    ax, 1                   
  01b60  f7d0               not    ax                      
  01b62  23d8               and    bx, ax                  
  01b64  8bc2               mov    ax, dx                  
  01b66  23c5               and    ax, bp                  
  01b68  0bc3               or     ax, bx                  
  01b6a  86c4               xchg   ah, al                  
  01b6c  8905               mov    word ptr [di], ax       
  01b6e  83c702             add    di, 2                   
  01b71  e2d6               loop   0x1b49                  
  01b73  8b0e1505           mov    cx, word ptr [0x515]    
  01b77  8b3e1305           mov    di, word ptr [0x513]    
  01b7b  81c7803e           add    di, 0x3e80              
  01b7f  ad                 lodsw  ax, word ptr [si]       
  01b80  86c4               xchg   ah, al                  
  01b82  8bd0               mov    dx, ax                  
  01b84  8b1d               mov    bx, word ptr [di]       
  01b86  86df               xchg   bh, bl                  
  01b88  d1e8               shr    ax, 1                   
  01b8a  d1e8               shr    ax, 1                   
  01b8c  f7d0               not    ax                      
  01b8e  23d8               and    bx, ax                  
  01b90  8bc2               mov    ax, dx                  
  01b92  d1e0               shl    ax, 1                   
  01b94  d1e0               shl    ax, 1                   
  01b96  f7d0               not    ax                      
  01b98  23d8               and    bx, ax                  
  01b9a  8bc2               mov    ax, dx                  
  01b9c  23c5               and    ax, bp                  
  01b9e  0bc3               or     ax, bx                  
  01ba0  86c4               xchg   ah, al                  
  01ba2  8905               mov    word ptr [di], ax       
  01ba4  83c702             add    di, 2                   
  01ba7  e2d6               loop   0x1b7f                  
  01ba9  8306130550         add    word ptr [0x513], 0x50  
  01bae  ff0e1905           dec    word ptr [0x519]        
  01bb2  833e190500         cmp    word ptr [0x519], 0     
  01bb7  7402               je     0x1bbb                  
  01bb9  eb86               jmp    0x1b41                  
  01bbb  07                 pop    es                      
  01bbc  5d                 pop    bp                      
  01bbd  5f                 pop    di                      
  01bbe  5e                 pop    si                      
  01bbf  c3                 ret                            
  01bc0  8b3e1305           mov    di, word ptr [0x513]    
  01bc4  8b0e1505           mov    cx, word ptr [0x515]    
  01bc8  ad                 lodsw  ax, word ptr [si]       
  01bc9  86c4               xchg   ah, al                  
  01bcb  8bd0               mov    dx, ax                  
  01bcd  8b1d               mov    bx, word ptr [di]       
  01bcf  86df               xchg   bh, bl                  
  01bd1  d1e8               shr    ax, 1                   
  01bd3  d1e8               shr    ax, 1                   
  01bd5  0bd8               or     bx, ax                  
  01bd7  8bc2               mov    ax, dx                  
  01bd9  d1e0               shl    ax, 1                   
  01bdb  d1e0               shl    ax, 1                   
  01bdd  0bd8               or     bx, ax                  
  01bdf  8bc2               mov    ax, dx                  
  01be1  f7d0               not    ax                      
  01be3  23d8               and    bx, ax                  
  01be5  86df               xchg   bh, bl                  
  01be7  891d               mov    word ptr [di], bx       
  01be9  83c702             add    di, 2                   
  01bec  e2da               loop   0x1bc8                  
  01bee  8b0e1505           mov    cx, word ptr [0x515]    
  01bf2  8b3e1305           mov    di, word ptr [0x513]    
  01bf6  81c7803e           add    di, 0x3e80              
  01bfa  ad                 lodsw  ax, word ptr [si]       
  01bfb  86c4               xchg   ah, al                  
  01bfd  8bd0               mov    dx, ax                  
  01bff  8b1d               mov    bx, word ptr [di]       
  01c01  86df               xchg   bh, bl                  
  01c03  d1e8               shr    ax, 1                   
  01c05  d1e8               shr    ax, 1                   
  01c07  0bd8               or     bx, ax                  
  01c09  8bc2               mov    ax, dx                  
  01c0b  d1e0               shl    ax, 1                   
  01c0d  d1e0               shl    ax, 1                   
  01c0f  0bd8               or     bx, ax                  
  01c11  8bc2               mov    ax, dx                  
  01c13  f7d0               not    ax                      
  01c15  23d8               and    bx, ax                  
  01c17  86df               xchg   bh, bl                  
  01c19  891d               mov    word ptr [di], bx       
  01c1b  83c702             add    di, 2                   
  01c1e  e2da               loop   0x1bfa                  
  01c20  8306130550         add    word ptr [0x513], 0x50  
  01c25  ff0e1905           dec    word ptr [0x519]        
  01c29  833e190500         cmp    word ptr [0x519], 0     
  01c2e  7590               jne    0x1bc0                  
  01c30  eb89               jmp    0x1bbb                  
  01c32  55                 push   bp                      
  01c33  8bec               mov    bp, sp                  
  01c35  8b7604             mov    si, word ptr [bp + 4]   
  01c38  8b7e06             mov    di, word ptr [bp + 6]   
  01c3b  8bd7               mov    dx, di                  
  01c3d  06                 push   es                      
  01c3e  8cd8               mov    ax, ds                  
  01c40  8ec0               mov    es, ax                  
  01c42  fc                 cld                            
  01c43  46                 inc    si                      
  01c44  46                 inc    si                      
  01c45  bd0400             mov    bp, 4                   
  01c48  ad                 lodsw  ax, word ptr [si]       
  01c49  8bd8               mov    bx, ax                  
  01c4b  8bfa               mov    di, dx                  
  01c4d  8bcd               mov    cx, bp                  
  01c4f  f3a5               rep movsw word ptr es:[di], word ptr [si]
  01c51  8bcd               mov    cx, bp                  
  01c53  8bfa               mov    di, dx                  
  01c55  81c7803e           add    di, 0x3e80              
  01c59  f3a5               rep movsw word ptr es:[di], word ptr [si]
  01c5b  83c250             add    dx, 0x50                
  01c5e  4b                 dec    bx                      
  01c5f  83fb00             cmp    bx, 0                   
  01c62  75e7               jne    0x1c4b                  
  01c64  07                 pop    es                      
  01c65  5d                 pop    bp                      
  01c66  c3                 ret                            
  01c67  55                 push   bp                      
  01c68  8bec               mov    bp, sp                  
  01c6a  8b7e04             mov    di, word ptr [bp + 4]   
  01c6d  d1e7               shl    di, 1                   
  01c6f  8bc7               mov    ax, di                  
  01c71  8b9d1ea5           mov    bx, word ptr [di - 0x5ae2]
  01c75  d1e7               shl    di, 1                   
  01c77  03f8               add    di, ax                  
  01c79  8db546a5           lea    si, [di - 0x5aba]       
  01c7d  b90300             mov    cx, 3                   
  01c80  8a07               mov    al, byte ptr [bx]       
  01c82  8804               mov    byte ptr [si], al       
  01c84  46                 inc    si                      
  01c85  8a87803e           mov    al, byte ptr [bx + 0x3e80]
  01c89  8804               mov    byte ptr [si], al       
  01c8b  46                 inc    si                      
  01c8c  83c350             add    bx, 0x50                
  01c8f  e2ef               loop   0x1c80                  
  01c91  5d                 pop    bp                      
  01c92  c3                 ret                            
  01c93  55                 push   bp                      
  01c94  8bec               mov    bp, sp                  
  01c96  fc                 cld                            
  01c97  8b7e04             mov    di, word ptr [bp + 4]   
  01c9a  d1e7               shl    di, 1                   
  01c9c  8bc7               mov    ax, di                  
  01c9e  8b9d1ea5           mov    bx, word ptr [di - 0x5ae2]
  01ca2  d1e7               shl    di, 1                   
  01ca4  03f8               add    di, ax                  
  01ca6  8db546a5           lea    si, [di - 0x5aba]       
  01caa  b90300             mov    cx, 3                   
  01cad  ac                 lodsb  al, byte ptr [si]       
  01cae  8807               mov    byte ptr [bx], al       
  01cb0  ac                 lodsb  al, byte ptr [si]       
  01cb1  8887803e           mov    byte ptr [bx + 0x3e80], al
  01cb5  83c350             add    bx, 0x50                
  01cb8  e2f3               loop   0x1cad                  
  01cba  5d                 pop    bp                      
  01cbb  c3                 ret                            
  01cbc  55                 push   bp                      
  01cbd  8bec               mov    bp, sp                  
  01cbf  8b7e04             mov    di, word ptr [bp + 4]   
  01cc2  d1e7               shl    di, 1                   
  01cc4  8b9d1ea5           mov    bx, word ptr [di - 0x5ae2]
  01cc8  b03c               mov    al, 0x3c                
  01cca  0807               or     byte ptr [bx], al       
  01ccc  0887803e           or     byte ptr [bx + 0x3e80], al
  01cd0  83c350             add    bx, 0x50                
  01cd3  b0ff               mov    al, 0xff                
  01cd5  8807               mov    byte ptr [bx], al       
  01cd7  8887803e           mov    byte ptr [bx + 0x3e80], al
  01cdb  83c350             add    bx, 0x50                
  01cde  b03c               mov    al, 0x3c                
  01ce0  0807               or     byte ptr [bx], al       
  01ce2  0887803e           or     byte ptr [bx + 0x3e80], al
  01ce6  5d                 pop    bp                      
  01ce7  c3                 ret                            
  01ce8  06                 push   es                      
  01ce9  55                 push   bp                      
  01cea  8bec               mov    bp, sp                  
  01cec  fc                 cld                            
  01ced  a15909             mov    ax, word ptr [0x959]    
  01cf0  8ec0               mov    es, ax                  
  01cf2  8b1ebea5           mov    bx, word ptr [0xa5be]   
  01cf6  81c3c800           add    bx, 0xc8                
  01cfa  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  01cfe  b8ffff             mov    ax, 0xffff              
  01d01  b92800             mov    cx, 0x28                
  01d04  f3ab               rep stosw word ptr es:[di], ax    
  01d06  33c0               xor    ax, ax                  
  01d08  43                 inc    bx                      
  01d09  43                 inc    bx                      
  01d0a  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  01d0e  b92800             mov    cx, 0x28                
  01d11  f3ab               rep stosw word ptr es:[di], ax    
  01d13  43                 inc    bx                      
  01d14  43                 inc    bx                      
  01d15  8b5606             mov    dx, word ptr [bp + 6]   
  01d18  83fa00             cmp    dx, 0                   
  01d1b  740e               je     0x1d2b                  
  01d1d  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  01d21  b92800             mov    cx, 0x28                
  01d24  f3ab               rep stosw word ptr es:[di], ax    
  01d26  43                 inc    bx                      
  01d27  43                 inc    bx                      
  01d28  4a                 dec    dx                      
  01d29  75f2               jne    0x1d1d                  
  01d2b  5d                 pop    bp                      
  01d2c  07                 pop    es                      
  01d2d  c3                 ret                            
  01d2e  55                 push   bp                      
  01d2f  8bec               mov    bp, sp                  
  01d31  8b5e04             mov    bx, word ptr [bp + 4]   
  01d34  8b1f               mov    bx, word ptr [bx]       
  01d36  8bf3               mov    si, bx                  
  01d38  8b4606             mov    ax, word ptr [bp + 6]   
  01d3b  250f00             and    ax, 0xf                 
  01d3e  b90400             mov    cx, 4                   
  01d41  ba0f00             mov    dx, 0xf                 
  01d44  33d8               xor    bx, ax                  
  01d46  23da               and    bx, dx                  
  01d48  7419               je     0x1d63                  
  01d4a  d1e0               shl    ax, 1                   
  01d4c  d1e0               shl    ax, 1                   
  01d4e  d1e0               shl    ax, 1                   
  01d50  d1e0               shl    ax, 1                   
  01d52  d1e2               shl    dx, 1                   
  01d54  d1e2               shl    dx, 1                   
  01d56  d1e2               shl    dx, 1                   
  01d58  d1e2               shl    dx, 1                   
  01d5a  8bde               mov    bx, si                  
  01d5c  e2e6               loop   0x1d44                  
  01d5e  b80000             mov    ax, 0                   
  01d61  eb03               jmp    0x1d66                  
  01d63  b80100             mov    ax, 1                   
  01d66  5d                 pop    bp                      
  01d67  c3                 ret                            
  01d68  06                 push   es                      
  01d69  fc                 cld                            
  01d6a  a15909             mov    ax, word ptr [0x959]    
  01d6d  8ec0               mov    es, ax                  
  01d6f  bbac00             mov    bx, 0xac                
  01d72  ba6800             mov    dx, 0x68                
  01d75  b0f0               mov    al, 0xf0                
  01d77  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  01d7b  aa                 stosb  byte ptr es:[di], al    
  01d7c  b90800             mov    cx, 8                   
  01d7f  b80000             mov    ax, 0                   
  01d82  f3ab               rep stosw word ptr es:[di], ax    
  01d84  43                 inc    bx                      
  01d85  43                 inc    bx                      
  01d86  4a                 dec    dx                      
  01d87  75ec               jne    0x1d75                  
  01d89  07                 pop    es                      
  01d8a  c3                 ret                            
  01d8b  55                 push   bp                      
  01d8c  8bec               mov    bp, sp                  
  01d8e  06                 push   es                      
  01d8f  fc                 cld                            
  01d90  a15909             mov    ax, word ptr [0x959]    
  01d93  8ec0               mov    es, ax                  
  01d95  8b5e06             mov    bx, word ptr [bp + 6]   
  01d98  d1e3               shl    bx, 1                   
  01d9a  ba0800             mov    dx, 8                   
  01d9d  b80000             mov    ax, 0                   
  01da0  b91900             mov    cx, 0x19                
  01da3  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  01da7  037e04             add    di, word ptr [bp + 4]   
  01daa  f3ab               rep stosw word ptr es:[di], ax    
  01dac  43                 inc    bx                      
  01dad  43                 inc    bx                      
  01dae  4a                 dec    dx                      
  01daf  75ef               jne    0x1da0                  
  01db1  8b7606             mov    si, word ptr [bp + 6]   
  01db4  d1e6               shl    si, 1                   
  01db6  8b5e08             mov    bx, word ptr [bp + 8]   
  01db9  ba0600             mov    dx, 6                   
  01dbc  b055               mov    al, 0x55                
  01dbe  8bcb               mov    cx, bx                  
  01dc0  8bbc6a09           mov    di, word ptr [si + 0x96a]
  01dc4  037e04             add    di, word ptr [bp + 4]   
  01dc7  f3aa               rep stosb byte ptr es:[di], al    
  01dc9  46                 inc    si                      
  01dca  46                 inc    si                      
  01dcb  4a                 dec    dx                      
  01dcc  75f0               jne    0x1dbe                  
  01dce  07                 pop    es                      
  01dcf  5d                 pop    bp                      
  01dd0  c3                 ret                            
  01dd1  55                 push   bp                      
  01dd2  8bec               mov    bp, sp                  
  01dd4  06                 push   es                      
  01dd5  fc                 cld                            
  01dd6  a15909             mov    ax, word ptr [0x959]    
  01dd9  8ec0               mov    es, ax                  
  01ddb  8b5604             mov    dx, word ptr [bp + 4]   
  01dde  8b5e06             mov    bx, word ptr [bp + 6]   
  01de1  d1e3               shl    bx, 1                   
  01de3  8b4e08             mov    cx, word ptr [bp + 8]   
  01de6  b00f               mov    al, 0xf                 
  01de8  e86a02             call   0x2055                  
  01deb  aa                 stosb  byte ptr es:[di], al    
  01dec  43                 inc    bx                      
  01ded  43                 inc    bx                      
  01dee  e2f8               loop   0x1de8                  
  01df0  07                 pop    es                      
  01df1  5d                 pop    bp                      
  01df2  c3                 ret                            
  01df3  55                 push   bp                      
  01df4  8bec               mov    bp, sp                  
  01df6  06                 push   es                      
  01df7  fc                 cld                            
  01df8  a15909             mov    ax, word ptr [0x959]    
  01dfb  8ec0               mov    es, ax                  
  01dfd  8b5604             mov    dx, word ptr [bp + 4]   
  01e00  8b5e06             mov    bx, word ptr [bp + 6]   
  01e03  d1e3               shl    bx, 1                   
  01e05  e84d02             call   0x2055                  
  01e08  8b4e08             mov    cx, word ptr [bp + 8]   
  01e0b  b0ff               mov    al, 0xff                
  01e0d  f3aa               rep stosb byte ptr es:[di], al    
  01e0f  07                 pop    es                      
  01e10  5d                 pop    bp                      
  01e11  c3                 ret                            
  01e12  55                 push   bp                      
  01e13  8bec               mov    bp, sp                  
  01e15  8b5604             mov    dx, word ptr [bp + 4]   
  01e18  8b5e06             mov    bx, word ptr [bp + 6]   
  01e1b  d1e3               shl    bx, 1                   
  01e1d  8b7608             mov    si, word ptr [bp + 8]   
  01e20  06                 push   es                      
  01e21  a15909             mov    ax, word ptr [0x959]    
  01e24  8ec0               mov    es, ax                  
  01e26  fc                 cld                            
  01e27  b90600             mov    cx, 6                   
  01e2a  e82802             call   0x2055                  
  01e2d  a5                 movsw  word ptr es:[di], word ptr [si]
  01e2e  43                 inc    bx                      
  01e2f  43                 inc    bx                      
  01e30  e2f8               loop   0x1e2a                  
  01e32  07                 pop    es                      
  01e33  5d                 pop    bp                      
  01e34  c3                 ret                            
  01e35  06                 push   es                      
  01e36  fc                 cld                            
  01e37  a15909             mov    ax, word ptr [0x959]    
  01e3a  8ec0               mov    es, ax                  
  01e3c  bbd400             mov    bx, 0xd4                
  01e3f  ba5400             mov    dx, 0x54                
  01e42  b0f0               mov    al, 0xf0                
  01e44  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  01e48  aa                 stosb  byte ptr es:[di], al    
  01e49  b90500             mov    cx, 5                   
  01e4c  b80000             mov    ax, 0                   
  01e4f  f3ab               rep stosw word ptr es:[di], ax    
  01e51  b00f               mov    al, 0xf                 
  01e53  aa                 stosb  byte ptr es:[di], al    
  01e54  43                 inc    bx                      
  01e55  43                 inc    bx                      
  01e56  4a                 dec    dx                      
  01e57  75e9               jne    0x1e42                  
  01e59  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  01e5d  b8ffff             mov    ax, 0xffff              
  01e60  b90600             mov    cx, 6                   
  01e63  f3ab               rep stosw word ptr es:[di], ax    
  01e65  07                 pop    es                      
  01e66  c3                 ret                            
  01e67  55                 push   bp                      
  01e68  8bec               mov    bp, sp                  
  01e6a  8b5604             mov    dx, word ptr [bp + 4]   
  01e6d  8b5e06             mov    bx, word ptr [bp + 6]   
  01e70  d1e3               shl    bx, 1                   
  01e72  06                 push   es                      
  01e73  a15909             mov    ax, word ptr [0x959]    
  01e76  8ec0               mov    es, ax                  
  01e78  8b4608             mov    ax, word ptr [bp + 8]   
  01e7b  8b4e0a             mov    cx, word ptr [bp + 0xa] 
  01e7e  e8d401             call   0x2055                  
  01e81  268805             mov    byte ptr es:[di], al    
  01e84  43                 inc    bx                      
  01e85  43                 inc    bx                      
  01e86  e8cc01             call   0x2055                  
  01e89  26880d             mov    byte ptr es:[di], cl    
  01e8c  43                 inc    bx                      
  01e8d  43                 inc    bx                      
  01e8e  e8c401             call   0x2055                  
  01e91  268805             mov    byte ptr es:[di], al    
  01e94  43                 inc    bx                      
  01e95  43                 inc    bx                      
  01e96  e8bc01             call   0x2055                  
  01e99  26880d             mov    byte ptr es:[di], cl    
  01e9c  07                 pop    es                      
  01e9d  5d                 pop    bp                      
  01e9e  c3                 ret                            
  01e9f  06                 push   es                      
  01ea0  55                 push   bp                      
  01ea1  fc                 cld                            
  01ea2  a15909             mov    ax, word ptr [0x959]    
  01ea5  8ec0               mov    es, ax                  
  01ea7  bb5001             mov    bx, 0x150               
  01eaa  bd0800             mov    bp, 8                   
  01ead  8bb76a09           mov    si, word ptr [bx + 0x96a]
  01eb1  8bbf7a09           mov    di, word ptr [bx + 0x97a]
  01eb5  b92800             mov    cx, 0x28                
  01eb8  268b04             mov    ax, word ptr es:[si]    
  01ebb  ab                 stosw  word ptr es:[di], ax    
  01ebc  46                 inc    si                      
  01ebd  46                 inc    si                      
  01ebe  e2f8               loop   0x1eb8                  
  01ec0  43                 inc    bx                      
  01ec1  43                 inc    bx                      
  01ec2  4d                 dec    bp                      
  01ec3  75e8               jne    0x1ead                  
  01ec5  bb5001             mov    bx, 0x150               
  01ec8  bd0800             mov    bp, 8                   
  01ecb  33c0               xor    ax, ax                  
  01ecd  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  01ed1  b92800             mov    cx, 0x28                
  01ed4  f3ab               rep stosw word ptr es:[di], ax    
  01ed6  43                 inc    bx                      
  01ed7  43                 inc    bx                      
  01ed8  4d                 dec    bp                      
  01ed9  75f2               jne    0x1ecd                  
  01edb  5d                 pop    bp                      
  01edc  07                 pop    es                      
  01edd  c3                 ret                            
  01ede  55                 push   bp                      
  01edf  8bec               mov    bp, sp                  
  01ee1  06                 push   es                      
  01ee2  fc                 cld                            
  01ee3  a15909             mov    ax, word ptr [0x959]    
  01ee6  8ec0               mov    es, ax                  
  01ee8  8b5e04             mov    bx, word ptr [bp + 4]   
  01eeb  d1e3               shl    bx, 1                   
  01eed  d1e3               shl    bx, 1                   
  01eef  d1e3               shl    bx, 1                   
  01ef1  d1e3               shl    bx, 1                   
  01ef3  33c0               xor    ax, ax                  
  01ef5  ba0800             mov    dx, 8                   
  01ef8  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  01efc  b92800             mov    cx, 0x28                
  01eff  f3ab               rep stosw word ptr es:[di], ax    
  01f01  43                 inc    bx                      
  01f02  43                 inc    bx                      
  01f03  4a                 dec    dx                      
  01f04  75f2               jne    0x1ef8                  
  01f06  8b4604             mov    ax, word ptr [bp + 4]   
  01f09  50                 push   ax                      
  01f0a  33c0               xor    ax, ax                  
  01f0c  50                 push   ax                      
  01f0d  e84c01             call   0x205c                  
  01f10  58                 pop    ax                      
  01f11  58                 pop    ax                      
  01f12  07                 pop    es                      
  01f13  5d                 pop    bp                      
  01f14  c3                 ret                            
  01f15  55                 push   bp                      
  01f16  8bec               mov    bp, sp                  
  01f18  06                 push   es                      
  01f19  fc                 cld                            
  01f1a  8cd8               mov    ax, ds                  
  01f1c  8ec0               mov    es, ax                  
  01f1e  8b4604             mov    ax, word ptr [bp + 4]   
  01f21  8b7e06             mov    di, word ptr [bp + 6]   
  01f24  8b4e08             mov    cx, word ptr [bp + 8]   
  01f27  f3aa               rep stosb byte ptr es:[di], al    
  01f29  07                 pop    es                      
  01f2a  5d                 pop    bp                      
  01f2b  c3                 ret                            
  01f2c  55                 push   bp                      
  01f2d  8bec               mov    bp, sp                  
  01f2f  06                 push   es                      
  01f30  fc                 cld                            
  01f31  8cd8               mov    ax, ds                  
  01f33  8ec0               mov    es, ax                  
  01f35  8b4604             mov    ax, word ptr [bp + 4]   
  01f38  8b7e06             mov    di, word ptr [bp + 6]   
  01f3b  8b4e08             mov    cx, word ptr [bp + 8]   
  01f3e  f3ab               rep stosw word ptr es:[di], ax    
  01f40  07                 pop    es                      
  01f41  5d                 pop    bp                      
  01f42  c3                 ret                            
  01f43  55                 push   bp                      
  01f44  8bec               mov    bp, sp                  
  01f46  06                 push   es                      
  01f47  fc                 cld                            
  01f48  8cd8               mov    ax, ds                  
  01f4a  8ec0               mov    es, ax                  
  01f4c  8b7604             mov    si, word ptr [bp + 4]   
  01f4f  8b7e06             mov    di, word ptr [bp + 6]   
  01f52  8b4e08             mov    cx, word ptr [bp + 8]   
  01f55  f3a4               rep movsb byte ptr es:[di], byte ptr [si]
  01f57  07                 pop    es                      
  01f58  5d                 pop    bp                      
  01f59  c3                 ret                            
  01f5a  55                 push   bp                      
  01f5b  8bec               mov    bp, sp                  
  01f5d  8b7604             mov    si, word ptr [bp + 4]   
  01f60  8b5606             mov    dx, word ptr [bp + 6]   
  01f63  8b5e08             mov    bx, word ptr [bp + 8]   
  01f66  d1e3               shl    bx, 1                   
  01f68  06                 push   es                      
  01f69  a15909             mov    ax, word ptr [0x959]    
  01f6c  8ec0               mov    es, ax                  
  01f6e  fc                 cld                            
  01f6f  ad                 lodsw  ax, word ptr [si]       
  01f70  a31305             mov    word ptr [0x513], ax    
  01f73  ad                 lodsw  ax, word ptr [si]       
  01f74  d1e0               shl    ax, 1                   
  01f76  e8dc00             call   0x2055                  
  01f79  8b0e1305           mov    cx, word ptr [0x513]    
  01f7d  f3a5               rep movsw word ptr es:[di], word ptr [si]
  01f7f  43                 inc    bx                      
  01f80  43                 inc    bx                      
  01f81  48                 dec    ax                      
  01f82  75f2               jne    0x1f76                  
  01f84  07                 pop    es                      
  01f85  5d                 pop    bp                      
  01f86  c3                 ret                            
  01f87  55                 push   bp                      
  01f88  8bec               mov    bp, sp                  
  01f8a  06                 push   es                      
  01f8b  a15909             mov    ax, word ptr [0x959]    
  01f8e  8ec0               mov    es, ax                  
  01f90  8b7e06             mov    di, word ptr [bp + 6]   
  01f93  d1e7               shl    di, 1                   
  01f95  d1e7               shl    di, 1                   
  01f97  d1e7               shl    di, 1                   
  01f99  d1e7               shl    di, 1                   
  01f9b  893e1305           mov    word ptr [0x513], di    
  01f9f  8b5604             mov    dx, word ptr [bp + 4]   
  01fa2  d1e2               shl    dx, 1                   
  01fa4  89161505           mov    word ptr [0x515], dx    
  01fa8  8d3e1b05           lea    di, [0x51b]             
  01fac  b608               mov    dh, 8                   
  01fae  8b361305           mov    si, word ptr [0x513]    
  01fb2  8bb46a09           mov    si, word ptr [si + 0x96a]
  01fb6  03361505           add    si, word ptr [0x515]    
  01fba  e83e00             call   0x1ffb                  
  01fbd  ff061305           inc    word ptr [0x513]        
  01fc1  ff061305           inc    word ptr [0x513]        
  01fc5  fece               dec    dh                      
  01fc7  75e5               jne    0x1fae                  
  01fc9  8d3ec905           lea    di, [0x5c9]             
  01fcd  1e                 push   ds                      
  01fce  07                 pop    es                      
  01fcf  8d361b05           lea    si, [0x51b]             
  01fd3  fc                 cld                            
  01fd4  b000               mov    al, 0                   
  01fd6  ba6100             mov    dx, 0x61                
  01fd9  56                 push   si                      
  01fda  57                 push   di                      
  01fdb  b90800             mov    cx, 8                   
  01fde  f3a6               repe cmpsb byte ptr [si], byte ptr es:[di]
  01fe0  5f                 pop    di                      
  01fe1  5e                 pop    si                      
  01fe2  7408               je     0x1fec                  
  01fe4  fec0               inc    al                      
  01fe6  83c708             add    di, 8                   
  01fe9  4a                 dec    dx                      
  01fea  75ed               jne    0x1fd9                  
  01fec  b400               mov    ah, 0                   
  01fee  052000             add    ax, 0x20                
  01ff1  3d7a00             cmp    ax, 0x7a                
  01ff4  7e02               jle    0x1ff8                  
  01ff6  33c0               xor    ax, ax                  
  01ff8  07                 pop    es                      
  01ff9  5d                 pop    bp                      
  01ffa  c3                 ret                            
  01ffb  268b04             mov    ax, word ptr es:[si]    
  01ffe  86e0               xchg   al, ah                  
  02000  b900c0             mov    cx, 0xc000              
  02003  b200               mov    dl, 0                   
  02005  85c1               test   cx, ax                  
  02007  f8                 clc                            
  02008  7401               je     0x200b                  
  0200a  f9                 stc                            
  0200b  d0d2               rcl    dl, 1                   
  0200d  d1e9               shr    cx, 1                   
  0200f  d1e9               shr    cx, 1                   
  02011  73f2               jae    0x2005                  
  02013  8815               mov    byte ptr [di], dl       
  02015  47                 inc    di                      
  02016  c3                 ret                            
  02017  55                 push   bp                      
  02018  8bec               mov    bp, sp                  
  0201a  06                 push   es                      
  0201b  833ee7a600         cmp    word ptr [0xa6e7], 0    
  02020  7430               je     0x2052                  
  02022  813ee7a60f27       cmp    word ptr [0xa6e7], 0x270f
  02028  7428               je     0x2052                  
  0202a  fc                 cld                            
  0202b  a15909             mov    ax, word ptr [0x959]    
  0202e  8ec0               mov    es, ax                  
  02030  8b5604             mov    dx, word ptr [bp + 4]   
  02033  d1e2               shl    dx, 1                   
  02035  bb8001             mov    bx, 0x180               
  02038  8b7606             mov    si, word ptr [bp + 6]   
  0203b  bd0800             mov    bp, 8                   
  0203e  b8ffff             mov    ax, 0xffff              
  02041  8bce               mov    cx, si                  
  02043  e80f00             call   0x2055                  
  02046  263105             xor    word ptr es:[di], ax    
  02049  47                 inc    di                      
  0204a  47                 inc    di                      
  0204b  e2f9               loop   0x2046                  
  0204d  43                 inc    bx                      
  0204e  43                 inc    bx                      
  0204f  4d                 dec    bp                      
  02050  75ef               jne    0x2041                  
  02052  07                 pop    es                      
  02053  5d                 pop    bp                      
  02054  c3                 ret                            
  02055  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  02059  03fa               add    di, dx                  
  0205b  c3                 ret                            
  0205c  55                 push   bp                      
  0205d  8bec               mov    bp, sp                  
  0205f  50                 push   ax                      
  02060  53                 push   bx                      
  02061  52                 push   dx                      
  02062  8b4604             mov    ax, word ptr [bp + 4]   
  02065  8ad0               mov    dl, al                  
  02067  a35e05             mov    word ptr [0x55e], ax    
  0206a  8b4606             mov    ax, word ptr [bp + 6]   
  0206d  8af0               mov    dh, al                  
  0206f  a36005             mov    word ptr [0x560], ax    
  02072  33db               xor    bx, bx                  
  02074  b402               mov    ah, 2                   
  02076  cd10               int    0x10   ; BIOS video int10h AH=02 : set cursor pos
  02078  5a                 pop    dx                      
  02079  5b                 pop    bx                      
  0207a  58                 pop    ax                      
  0207b  5d                 pop    bp                      
  0207c  c3                 ret                            
  0207d  55                 push   bp                      
  0207e  8bec               mov    bp, sp                  
  02080  8b4e04             mov    cx, word ptr [bp + 4]   
  02083  83f900             cmp    cx, 0                   
  02086  7e0c               jle    0x2094                  
  02088  b82000             mov    ax, 0x20                
  0208b  51                 push   cx                      
  0208c  50                 push   ax                      
  0208d  e85704             call   0x24e7                  
  02090  58                 pop    ax                      
  02091  59                 pop    cx                      
  02092  e2f4               loop   0x2088                  
  02094  5d                 pop    bp                      
  02095  c3                 ret                            
  02096  1e                 push   ds                      
  02097  b84000             mov    ax, 0x40                
  0209a  8ed8               mov    ds, ax                  
  0209c  a06c00             mov    al, byte ptr [0x6c]     
  0209f  1f                 pop    ds                      
  020a0  a33505             mov    word ptr [0x535], ax    
  020a3  c7063705b80b       mov    word ptr [0x537], 0xbb8 
  020a9  c3                 ret                            
  020aa  ff063705           inc    word ptr [0x537]        
  020ae  813e37052823       cmp    word ptr [0x537], 0x2328
  020b4  7506               jne    0x20bc                  
  020b6  c7063705b80b       mov    word ptr [0x537], 0xbb8 
  020bc  8b1e3705           mov    bx, word ptr [0x537]    
  020c0  2e8b07             mov    ax, word ptr cs:[bx]    
  020c3  03063505           add    ax, word ptr [0x535]    
  020c7  257f00             and    ax, 0x7f                
  020ca  a33505             mov    word ptr [0x535], ax    
  020cd  c3                 ret                            
  020ce  833ee1a600         cmp    word ptr [0xa6e1], 0    
  020d3  7406               je     0x20db                  
  020d5  e461               in     al, 0x61                
  020d7  3402               xor    al, 2                   
  020d9  e661               out    0x61, al                
  020db  c3                 ret                            
  020dc  55                 push   bp                      
  020dd  8bec               mov    bp, sp                  
  020df  b0b6               mov    al, 0xb6                
  020e1  e643               out    0x43, al                
  020e3  8b4604             mov    ax, word ptr [bp + 4]   
  020e6  e642               out    0x42, al                
  020e8  8ac4               mov    al, ah                  
  020ea  90                 nop                            
  020eb  e642               out    0x42, al                
  020ed  5d                 pop    bp                      
  020ee  c3                 ret                            
  020ef  833ee1a600         cmp    word ptr [0xa6e1], 0    
  020f4  7406               je     0x20fc                  
  020f6  e461               in     al, 0x61                
  020f8  0c03               or     al, 3                   
  020fa  e661               out    0x61, al                
  020fc  c3                 ret                            
  020fd  e461               in     al, 0x61                
  020ff  24fc               and    al, 0xfc                
  02101  e661               out    0x61, al                
  02103  c3                 ret                            
  02104  55                 push   bp                      
  02105  8bec               mov    bp, sp                  
  02107  8b5e04             mov    bx, word ptr [bp + 4]   
  0210a  d1e3               shl    bx, 1                   
  0210c  d1e3               shl    bx, 1                   
  0210e  d1e3               shl    bx, 1                   
  02110  d1e3               shl    bx, 1                   
  02112  d1e3               shl    bx, 1                   
  02114  8ac3               mov    al, bl                  
  02116  8b5e06             mov    bx, word ptr [bp + 6]   
  02119  80e30f             and    bl, 0xf                 
  0211c  0ac3               or     al, bl                  
  0211e  0c80               or     al, 0x80                
  02120  e6c0               out    0xc0, al                
  02122  8b5e06             mov    bx, word ptr [bp + 6]   
  02125  d1eb               shr    bx, 1                   
  02127  d1eb               shr    bx, 1                   
  02129  d1eb               shr    bx, 1                   
  0212b  d1eb               shr    bx, 1                   
  0212d  8ac3               mov    al, bl                  
  0212f  247f               and    al, 0x7f                
  02131  e6c0               out    0xc0, al                
  02133  5d                 pop    bp                      
  02134  c3                 ret                            
  02135  55                 push   bp                      
  02136  8bec               mov    bp, sp                  
  02138  8b4604             mov    ax, word ptr [bp + 4]   
  0213b  d0e0               shl    al, 1                   
  0213d  d0e0               shl    al, 1                   
  0213f  d0e0               shl    al, 1                   
  02141  d0e0               shl    al, 1                   
  02143  d0e0               shl    al, 1                   
  02145  8b5e06             mov    bx, word ptr [bp + 6]   
  02148  80e30f             and    bl, 0xf                 
  0214b  0ac3               or     al, bl                  
  0214d  0c90               or     al, 0x90                
  0214f  e6c0               out    0xc0, al                
  02151  5d                 pop    bp                      
  02152  c3                 ret                            
  02153  55                 push   bp                      
  02154  8bec               mov    bp, sp                  
  02156  8b4604             mov    ax, word ptr [bp + 4]   
  02159  250f00             and    ax, 0xf                 
  0215c  0de000             or     ax, 0xe0                
  0215f  e6c0               out    0xc0, al                
  02161  5d                 pop    bp                      
  02162  c3                 ret                            
  02163  833ee1a600         cmp    word ptr [0xa6e1], 0    
  02168  7406               je     0x2170                  
  0216a  e461               in     al, 0x61                
  0216c  0c60               or     al, 0x60                
  0216e  e661               out    0x61, al                
  02170  c3                 ret                            
  02171  b09f               mov    al, 0x9f                
  02173  e6c0               out    0xc0, al                
  02175  b0bf               mov    al, 0xbf                
  02177  e6c0               out    0xc0, al                
  02179  b0df               mov    al, 0xdf                
  0217b  e6c0               out    0xc0, al                
  0217d  b0ff               mov    al, 0xff                
  0217f  e6c0               out    0xc0, al                
  02181  e461               in     al, 0x61                
  02183  249f               and    al, 0x9f                
  02185  e661               out    0x61, al                
  02187  c3                 ret                            
  02188  55                 push   bp                      
  02189  8bec               mov    bp, sp                  
  0218b  51                 push   cx                      
  0218c  52                 push   dx                      
  0218d  8b4e04             mov    cx, word ptr [bp + 4]   
  02190  a14809             mov    ax, word ptr [0x948]    
  02193  8bd1               mov    dx, cx                  
  02195  d1ea               shr    dx, 1                   
  02197  3d2003             cmp    ax, 0x320               
  0219a  7704               ja     0x21a0                  
  0219c  8bca               mov    cx, dx                  
  0219e  eb0e               jmp    0x21ae                  
  021a0  2d8403             sub    ax, 0x384               
  021a3  7209               jb     0x21ae                  
  021a5  2df401             sub    ax, 0x1f4               
  021a8  7204               jb     0x21ae                  
  021aa  03ca               add    cx, dx                  
  021ac  ebf7               jmp    0x21a5                  
  021ae  83f900             cmp    cx, 0                   
  021b1  7503               jne    0x21b6                  
  021b3  b90100             mov    cx, 1                   
  021b6  e8f1fe             call   0x20aa                  
  021b9  e2fb               loop   0x21b6                  
  021bb  5a                 pop    dx                      
  021bc  59                 pop    cx                      
  021bd  5d                 pop    bp                      
  021be  c3                 ret                            
  021bf  fb                 sti                            
  021c0  8bec               mov    bp, sp                  
  021c2  1e                 push   ds                      
  021c3  53                 push   bx                      
  021c4  51                 push   cx                      
  021c5  52                 push   dx                      
  021c6  57                 push   di                      
  021c7  8b4614             mov    ax, word ptr [bp + 0x14]
  021ca  8ed8               mov    ds, ax                  
  021cc  e83f00             call   0x220e                  
  021cf  5f                 pop    di                      
  021d0  8bc7               mov    ax, di                  
  021d2  3c00               cmp    al, 0                   
  021d4  7506               jne    0x21dc                  
  021d6  8d069005           lea    ax, [0x590]             
  021da  eb04               jmp    0x21e0                  
  021dc  8d067b05           lea    ax, [0x57b]             
  021e0  e83700             call   0x221a                  
  021e3  833e2f0510         cmp    word ptr [0x52f], 0x10  
  021e8  7414               je     0x21fe                  
  021ea  833e2f0513         cmp    word ptr [0x52f], 0x13  
  021ef  75f2               jne    0x21e3                  
  021f1  c7062f050000       mov    word ptr [0x52f], 0     
  021f7  5a                 pop    dx                      
  021f8  59                 pop    cx                      
  021f9  5b                 pop    bx                      
  021fa  1f                 pop    ds                      
  021fb  b001               mov    al, 1                   
  021fd  cf                 iret                           
  021fe  c7062f050000       mov    word ptr [0x52f], 0     
  02204  83c41a             add    sp, 0x1a                
  02207  5d                 pop    bp                      
  02208  1f                 pop    ds                      
  02209  07                 pop    es                      
  0220a  b80f27             mov    ax, 0x270f              
  0220d  cf                 iret                           
  0220e  b81800             mov    ax, 0x18                
  02211  50                 push   ax                      
  02212  e8c9fc             call   0x1ede                  
  02215  58                 pop    ax                      
  02216  e8f300             call   0x230c                  
  02219  c3                 ret                            
  0221a  50                 push   ax                      
  0221b  e84703             call   0x2565                  
  0221e  58                 pop    ax                      
  0221f  8d06a505           lea    ax, [0x5a5]             
  02223  50                 push   ax                      
  02224  e83e03             call   0x2565                  
  02227  58                 pop    ax                      
  02228  c7062f050000       mov    word ptr [0x52f], 0     
  0222e  c3                 ret                            
  0222f  fb                 sti                            
  02230  8bec               mov    bp, sp                  
  02232  1e                 push   ds                      
  02233  53                 push   bx                      
  02234  51                 push   cx                      
  02235  52                 push   dx                      
  02236  57                 push   di                      
  02237  8b4614             mov    ax, word ptr [bp + 0x14]
  0223a  8ed8               mov    ds, ax                  
  0223c  5f                 pop    di                      
  0223d  8bc7               mov    ax, di                  
  0223f  3c00               cmp    al, 0                   
  02241  7535               jne    0x2278                  
  02243  e8c8ff             call   0x220e                  
  02246  8d069005           lea    ax, [0x590]             
  0224a  e8cdff             call   0x221a                  
  0224d  833e2f0510         cmp    word ptr [0x52f], 0x10  
  02252  7414               je     0x2268                  
  02254  833e2f0513         cmp    word ptr [0x52f], 0x13  
  02259  75f2               jne    0x224d                  
  0225b  c7062f050000       mov    word ptr [0x52f], 0     
  02261  5a                 pop    dx                      
  02262  59                 pop    cx                      
  02263  5b                 pop    bx                      
  02264  1f                 pop    ds                      
  02265  b001               mov    al, 1                   
  02267  cf                 iret                           
  02268  b80f27             mov    ax, 0x270f              
  0226b  c7062f050000       mov    word ptr [0x52f], 0     
  02271  83c41a             add    sp, 0x1a                
  02274  5d                 pop    bp                      
  02275  1f                 pop    ds                      
  02276  07                 pop    es                      
  02277  cf                 iret                           
  02278  33c0               xor    ax, ax                  
  0227a  83c41a             add    sp, 0x1a                
  0227d  5d                 pop    bp                      
  0227e  1f                 pop    ds                      
  0227f  07                 pop    es                      
  02280  f8                 clc                            
  02281  cf                 iret                           
  02282  fa                 cli                            
  02283  06                 push   es                      
  02284  b80000             mov    ax, 0                   
  02287  8ec0               mov    es, ax                  
  02289  a12705             mov    ax, word ptr [0x527]    
  0228c  26a31400           mov    word ptr es:[0x14], ax  
  02290  a12905             mov    ax, word ptr [0x529]    
  02293  26a31600           mov    word ptr es:[0x16], ax  
  02297  a12b05             mov    ax, word ptr [0x52b]    
  0229a  26a36c00           mov    word ptr es:[0x6c], ax  
  0229e  a12d05             mov    ax, word ptr [0x52d]    
  022a1  26a36e00           mov    word ptr es:[0x6e], ax  
  022a5  07                 pop    es                      
  022a6  fb                 sti                            
  022a7  8b1e3905           mov    bx, word ptr [0x539]    
  022ab  1e                 push   ds                      
  022ac  b800f0             mov    ax, 0xf000              
  022af  8ed8               mov    ds, ax                  
  022b1  a100c0             mov    ax, word ptr [0xc000]   
  022b4  3c21               cmp    al, 0x21                
  022b6  7409               je     0x22c1                  
  022b8  b84000             mov    ax, 0x40                
  022bb  8ed8               mov    ds, ax                  
  022bd  891e1000           mov    word ptr [0x10], bx     
  022c1  1f                 pop    ds                      
  022c2  a13b05             mov    ax, word ptr [0x53b]    
  022c5  cd10               int    0x10   ; BIOS video int10h AH=?? : (AH varies)
  022c7  c3                 ret                            
  022c8  55                 push   bp                      
  022c9  8bec               mov    bp, sp                  
  022cb  8b4604             mov    ax, word ptr [bp + 4]   
  022ce  3d0000             cmp    ax, 0                   
  022d1  7505               jne    0x22d8                  
  022d3  bb0001             mov    bx, 0x100               
  022d6  eb03               jmp    0x22db                  
  022d8  bb0101             mov    bx, 0x101               
  022db  b8000b             mov    ax, 0xb00               
  022de  cd10               int    0x10   ; BIOS video int10h AH=0b : set palette/border
  022e0  5d                 pop    bp                      
  022e1  c3                 ret                            
  022e2  55                 push   bp                      
  022e3  8bec               mov    bp, sp                  
  022e5  8b5e04             mov    bx, word ptr [bp + 4]   
  022e8  8b4606             mov    ax, word ptr [bp + 6]   
  022eb  8af8               mov    bh, al                  
  022ed  b410               mov    ah, 0x10                
  022ef  b000               mov    al, 0                   
  022f1  cd10               int    0x10   ; BIOS video int10h AH=10 : ?
  022f3  5d                 pop    bp                      
  022f4  c3                 ret                            
  022f5  e8f7fd             call   0x20ef                  
  022f8  b8e803             mov    ax, 0x3e8               
  022fb  50                 push   ax                      
  022fc  e8ddfd             call   0x20dc                  
  022ff  58                 pop    ax                      
  02300  b84605             mov    ax, 0x546               
  02303  50                 push   ax                      
  02304  e881fe             call   0x2188                  
  02307  58                 pop    ax                      
  02308  e8f2fd             call   0x20fd                  
  0230b  c3                 ret                            
  0230c  e8e0fd             call   0x20ef                  
  0230f  b88813             mov    ax, 0x1388              
  02312  50                 push   ax                      
  02313  e8c6fd             call   0x20dc                  
  02316  58                 pop    ax                      
  02317  b8b80b             mov    ax, 0xbb8               
  0231a  50                 push   ax                      
  0231b  e86afe             call   0x2188                  
  0231e  58                 pop    ax                      
  0231f  e8dbfd             call   0x20fd                  
  02322  c3                 ret                            
  02323  fa                 cli                            
  02324  06                 push   es                      
  02325  b80000             mov    ax, 0                   
  02328  8ec0               mov    es, ax                  
  0232a  8cd8               mov    ax, ds                  
  0232c  26a31802           mov    word ptr es:[0x218], ax 
  02330  26a12400           mov    ax, word ptr es:[0x24]  
  02334  a32305             mov    word ptr [0x523], ax    
  02337  26a12600           mov    ax, word ptr es:[0x26]  
  0233b  a32505             mov    word ptr [0x525], ax    
  0233e  8d067323           lea    ax, [0x2373]            
  02342  26a32400           mov    word ptr es:[0x24], ax  
  02346  8cc8               mov    ax, cs                  
  02348  26a32600           mov    word ptr es:[0x26], ax  
  0234c  07                 pop    es                      
  0234d  fb                 sti                            
  0234e  c3                 ret                            
  0234f  fa                 cli                            
  02350  06                 push   es                      
  02351  b80000             mov    ax, 0                   
  02354  8ec0               mov    es, ax                  
  02356  a12305             mov    ax, word ptr [0x523]    
  02359  26a32400           mov    word ptr es:[0x24], ax  
  0235d  a12505             mov    ax, word ptr [0x525]    
  02360  26a32600           mov    word ptr es:[0x26], ax  
  02364  07                 pop    es                      
  02365  fb                 sti                            
  02366  1e                 push   ds                      
  02367  b84000             mov    ax, 0x40                
  0236a  8ed8               mov    ds, ax                  
  0236c  33c0               xor    ax, ax                  
  0236e  a31700             mov    word ptr [0x17], ax     
  02371  1f                 pop    ds                      
  02372  c3                 ret                            
  02373  fb                 sti                            
  02374  50                 push   ax                      
  02375  53                 push   bx                      
  02376  56                 push   si                      
  02377  1e                 push   ds                      
  02378  06                 push   es                      
  02379  33c0               xor    ax, ax                  
  0237b  8bd8               mov    bx, ax                  
  0237d  8ec0               mov    es, ax                  
  0237f  26a11802           mov    ax, word ptr es:[0x218] 
  02383  8ed8               mov    ds, ax                  
  02385  b84000             mov    ax, 0x40                
  02388  8ec0               mov    es, ax                  
  0238a  b0f7               mov    al, 0xf7                
  0238c  2620061800         and    byte ptr es:[0x18], al  
  02391  e460               in     al, 0x60                
  02393  50                 push   ax                      
  02394  e461               in     al, 0x61                
  02396  8ae0               mov    ah, al                  
  02398  0c80               or     al, 0x80                
  0239a  e661               out    0x61, al                
  0239c  86e0               xchg   al, ah                  
  0239e  e661               out    0x61, al                
  023a0  58                 pop    ax                      
  023a1  8ae0               mov    ah, al                  
  023a3  247f               and    al, 0x7f                
  023a5  833e430500         cmp    word ptr [0x543], 0     
  023aa  740c               je     0x23b8                  
  023ac  3c4a               cmp    al, 0x4a                
  023ae  7502               jne    0x23b2                  
  023b0  b04e               mov    al, 0x4e                
  023b2  3c29               cmp    al, 0x29                
  023b4  7502               jne    0x23b8                  
  023b6  b04a               mov    al, 0x4a                
  023b8  3c47               cmp    al, 0x47                
  023ba  7220               jb     0x23dc                  
  023bc  3c51               cmp    al, 0x51                
  023be  771c               ja     0x23dc                  
  023c0  3c4a               cmp    al, 0x4a                
  023c2  7418               je     0x23dc                  
  023c4  3c4e               cmp    al, 0x4e                
  023c6  7414               je     0x23dc                  
  023c8  8ad8               mov    bl, al                  
  023ca  80eb47             sub    bl, 0x47                
  023cd  80fb03             cmp    bl, 3                   
  023d0  7209               jb     0x23db                  
  023d2  fecb               dec    bl                      
  023d4  80fb06             cmp    bl, 6                   
  023d7  7202               jb     0x23db                  
  023d9  fecb               dec    bl                      
  023db  43                 inc    bx                      
  023dc  80fc80             cmp    ah, 0x80                
  023df  b400               mov    ah, 0                   
  023e1  724f               jb     0x2432                  
  023e3  83fb00             cmp    bx, 0                   
  023e6  740e               je     0x23f6                  
  023e8  4b                 dec    bx                      
  023e9  33c0               xor    ax, ax                  
  023eb  88875505           mov    byte ptr [bx + 0x555], al
  023ef  e89b00             call   0x248d                  
  023f2  33db               xor    bx, bx                  
  023f4  eb3c               jmp    0x2432                  
  023f6  3c2a               cmp    al, 0x2a                
  023f8  7404               je     0x23fe                  
  023fa  3c36               cmp    al, 0x36                
  023fc  7508               jne    0x2406                  
  023fe  c70647050000       mov    word ptr [0x547], 0     
  02404  eb22               jmp    0x2428                  
  02406  3c38               cmp    al, 0x38                
  02408  7508               jne    0x2412                  
  0240a  c70649050000       mov    word ptr [0x549], 0     
  02410  eb16               jmp    0x2428                  
  02412  3c1d               cmp    al, 0x1d                
  02414  7508               jne    0x241e                  
  02416  c7064b050000       mov    word ptr [0x54b], 0     
  0241c  eb0a               jmp    0x2428                  
  0241e  3c53               cmp    al, 0x53                
  02420  7506               jne    0x2428                  
  02422  c7064d050000       mov    word ptr [0x54d], 0     
  02428  3b062f05           cmp    ax, word ptr [0x52f]    
  0242c  7554               jne    0x2482                  
  0242e  b000               mov    al, 0                   
  02430  eb45               jmp    0x2477                  
  02432  83fb00             cmp    bx, 0                   
  02435  7409               je     0x2440                  
  02437  4b                 dec    bx                      
  02438  c687550501         mov    byte ptr [bx + 0x555], 1
  0243d  e84d00             call   0x248d                  
  02440  3d2a00             cmp    ax, 0x2a                
  02443  7405               je     0x244a                  
  02445  3d3600             cmp    ax, 0x36                
  02448  7508               jne    0x2452                  
  0244a  c70647050100       mov    word ptr [0x547], 1     
  02450  eb25               jmp    0x2477                  
  02452  3d3800             cmp    ax, 0x38                
  02455  7508               jne    0x245f                  
  02457  c70649050100       mov    word ptr [0x549], 1     
  0245d  eb18               jmp    0x2477                  
  0245f  3d1d00             cmp    ax, 0x1d                
  02462  7508               jne    0x246c                  
  02464  c7064b050100       mov    word ptr [0x54b], 1     
  0246a  eb0b               jmp    0x2477                  
  0246c  3d5300             cmp    ax, 0x53                
  0246f  7506               jne    0x2477                  
  02471  c7064d050100       mov    word ptr [0x54d], 1     
  02477  a32f05             mov    word ptr [0x52f], ax    
  0247a  3d0000             cmp    ax, 0                   
  0247d  7403               je     0x2482                  
  0247f  a33105             mov    word ptr [0x531], ax    
  02482  fa                 cli                            
  02483  b020               mov    al, 0x20                
  02485  e620               out    0x20, al                
  02487  07                 pop    es                      
  02488  1f                 pop    ds                      
  02489  5e                 pop    si                      
  0248a  5b                 pop    bx                      
  0248b  58                 pop    ax                      
  0248c  cf                 iret                           
  0248d  803e560501         cmp    byte ptr [0x556], 1     
  02492  7514               jne    0x24a8                  
  02494  803e5a0501         cmp    byte ptr [0x55a], 1     
  02499  7503               jne    0x249e                  
  0249b  b049               mov    al, 0x49                
  0249d  c3                 ret                            
  0249e  803e580501         cmp    byte ptr [0x558], 1     
  024a3  751e               jne    0x24c3                  
  024a5  b047               mov    al, 0x47                
  024a7  c3                 ret                            
  024a8  803e5c0501         cmp    byte ptr [0x55c], 1     
  024ad  7514               jne    0x24c3                  
  024af  803e5a0501         cmp    byte ptr [0x55a], 1     
  024b4  7503               jne    0x24b9                  
  024b6  b051               mov    al, 0x51                
  024b8  c3                 ret                            
  024b9  803e580501         cmp    byte ptr [0x558], 1     
  024be  7503               jne    0x24c3                  
  024c0  b04f               mov    al, 0x4f                
  024c2  c3                 ret                            
  024c3  be0800             mov    si, 8                   
  024c6  80bc550501         cmp    byte ptr [si + 0x555], 1
  024cb  7513               jne    0x24e0                  
  024cd  b047               mov    al, 0x47                
  024cf  03c6               add    ax, si                  
  024d1  83fe03             cmp    si, 3                   
  024d4  7210               jb     0x24e6                  
  024d6  fec0               inc    al                      
  024d8  83fe06             cmp    si, 6                   
  024db  7209               jb     0x24e6                  
  024dd  fec0               inc    al                      
  024df  c3                 ret                            
  024e0  4e                 dec    si                      
  024e1  83feff             cmp    si, -1                  
  024e4  75e0               jne    0x24c6                  
  024e6  c3                 ret                            
  024e7  55                 push   bp                      
  024e8  8bec               mov    bp, sp                  
  024ea  8b4e04             mov    cx, word ptr [bp + 4]   
  024ed  8b165e05           mov    dx, word ptr [0x55e]    
  024f1  a16005             mov    ax, word ptr [0x560]    
  024f4  83f90a             cmp    cx, 0xa                 
  024f7  7506               jne    0x24ff                  
  024f9  ff066005           inc    word ptr [0x560]        
  024fd  eb64               jmp    0x2563                  
  024ff  83f90d             cmp    cx, 0xd                 
  02502  7508               jne    0x250c                  
  02504  c7065e050000       mov    word ptr [0x55e], 0     
  0250a  eb57               jmp    0x2563                  
  0250c  83f908             cmp    cx, 8                   
  0250f  750d               jne    0x251e                  
  02511  833e5e0500         cmp    word ptr [0x55e], 0     
  02516  7e06               jle    0x251e                  
  02518  ff0e5e05           dec    word ptr [0x55e]        
  0251c  eb45               jmp    0x2563                  
  0251e  06                 push   es                      
  0251f  56                 push   si                      
  02520  57                 push   di                      
  02521  8bf8               mov    di, ax                  
  02523  e89d01             call   0x26c3                  
  02526  8bef               mov    bp, di                  
  02528  8bf1               mov    si, cx                  
  0252a  83ee20             sub    si, 0x20                
  0252d  d1e6               shl    si, 1                   
  0252f  d1e6               shl    si, 1                   
  02531  d1e6               shl    si, 1                   
  02533  8d0ec905           lea    cx, [0x5c9]             
  02537  03f1               add    si, cx                  
  02539  fc                 cld                            
  0253a  b90800             mov    cx, 8                   
  0253d  8b7e00             mov    di, word ptr [bp]       
  02540  03fa               add    di, dx                  
  02542  ac                 lodsb  al, byte ptr [si]       
  02543  e85f01             call   0x26a5                  
  02546  83c502             add    bp, 2                   
  02549  e2f2               loop   0x253d                  
  0254b  ff065e05           inc    word ptr [0x55e]        
  0254f  833e5e0528         cmp    word ptr [0x55e], 0x28  
  02554  720a               jb     0x2560                  
  02556  c7065e050000       mov    word ptr [0x55e], 0     
  0255c  ff066005           inc    word ptr [0x560]        
  02560  5f                 pop    di                      
  02561  5e                 pop    si                      
  02562  07                 pop    es                      
  02563  5d                 pop    bp                      
  02564  c3                 ret                            
  02565  55                 push   bp                      
  02566  8bec               mov    bp, sp                  
  02568  8b5e04             mov    bx, word ptr [bp + 4]   
  0256b  8a07               mov    al, byte ptr [bx]       
  0256d  3c00               cmp    al, 0                   
  0256f  740a               je     0x257b                  
  02571  b400               mov    ah, 0                   
  02573  50                 push   ax                      
  02574  e870ff             call   0x24e7                  
  02577  58                 pop    ax                      
  02578  43                 inc    bx                      
  02579  ebf0               jmp    0x256b                  
  0257b  5d                 pop    bp                      
  0257c  c3                 ret                            
  0257d  fa                 cli                            
  0257e  bb0f27             mov    bx, 0x270f              
  02581  891e4f05           mov    word ptr [0x54f], bx    
  02585  891e5105           mov    word ptr [0x551], bx    
  02589  e461               in     al, 0x61                
  0258b  0c01               or     al, 1                   
  0258d  e661               out    0x61, al                
  0258f  b0b4               mov    al, 0xb4                
  02591  e643               out    0x43, al                
  02593  33c0               xor    ax, ax                  
  02595  90                 nop                            
  02596  e642               out    0x42, al                
  02598  90                 nop                            
  02599  90                 nop                            
  0259a  e642               out    0x42, al                
  0259c  ba0102             mov    dx, 0x201               
  0259f  ee                 out    dx, al                  
  025a0  b98813             mov    cx, 0x1388              
  025a3  49                 dec    cx                      
  025a4  7439               je     0x25df                  
  025a6  ec                 in     al, dx                  
  025a7  a801               test   al, 1                   
  025a9  750c               jne    0x25b7                  
  025ab  391e4f05           cmp    word ptr [0x54f], bx    
  025af  7506               jne    0x25b7                  
  025b1  e83000             call   0x25e4                  
  025b4  a34f05             mov    word ptr [0x54f], ax    
  025b7  ec                 in     al, dx                  
  025b8  a802               test   al, 2                   
  025ba  75e7               jne    0x25a3                  
  025bc  391e5105           cmp    word ptr [0x551], bx    
  025c0  7506               jne    0x25c8                  
  025c2  e81f00             call   0x25e4                  
  025c5  a35105             mov    word ptr [0x551], ax    
  025c8  391e4f05           cmp    word ptr [0x54f], bx    
  025cc  74d5               je     0x25a3                  
  025ce  ec                 in     al, dx                  
  025cf  f7d0               not    ax                      
  025d1  253000             and    ax, 0x30                
  025d4  d0e8               shr    al, 1                   
  025d6  d0e8               shr    al, 1                   
  025d8  d0e8               shr    al, 1                   
  025da  d0e8               shr    al, 1                   
  025dc  a35305             mov    word ptr [0x553], ax    
  025df  fb                 sti                            
  025e0  e81afb             call   0x20fd                  
  025e3  c3                 ret                            
  025e4  b080               mov    al, 0x80                
  025e6  e643               out    0x43, al                
  025e8  90                 nop                            
  025e9  90                 nop                            
  025ea  e442               in     al, 0x42                
  025ec  8ae0               mov    ah, al                  
  025ee  90                 nop                            
  025ef  e442               in     al, 0x42                
  025f1  86c4               xchg   ah, al                  
  025f3  f7d8               neg    ax                      
  025f5  2d2800             sub    ax, 0x28                
  025f8  d1e8               shr    ax, 1                   
  025fa  d1e8               shr    ax, 1                   
  025fc  d1e8               shr    ax, 1                   
  025fe  d1e8               shr    ax, 1                   
  02600  d1e8               shr    ax, 1                   
  02602  3df401             cmp    ax, 0x1f4               
  02605  7c02               jl     0x2609                  
  02607  33c0               xor    ax, ax                  
  02609  c3                 ret                            
  0260a  1e                 push   ds                      
  0260b  ba2f22             mov    dx, 0x222f              
  0260e  0e                 push   cs                      
  0260f  1f                 pop    ds                      
  02610  b024               mov    al, 0x24                
  02612  b425               mov    ah, 0x25                
  02614  cd21               int    0x21   ; DOS int21h AH=25 : set int vector
  02616  1f                 pop    ds                      
  02617  55                 push   bp                      
  02618  8bec               mov    bp, sp                  
  0261a  8b5604             mov    dx, word ptr [bp + 4]   
  0261d  b002               mov    al, 2                   
  0261f  b43d               mov    ah, 0x3d                
  02621  cd21               int    0x21   ; DOS int21h AH=3d : OPEN FILE (DS:DX name,AL mode)
  02623  7318               jae    0x263d                  
  02625  3d0f27             cmp    ax, 0x270f              
  02628  7413               je     0x263d                  
  0262a  8b4606             mov    ax, word ptr [bp + 6]   
  0262d  3d0500             cmp    ax, 5                   
  02630  7509               jne    0x263b                  
  02632  b90000             mov    cx, 0                   
  02635  b43c               mov    ah, 0x3c                
  02637  cd21               int    0x21   ; DOS int21h AH=3c : create file
  02639  7302               jae    0x263d                  
  0263b  33c0               xor    ax, ax                  
  0263d  5d                 pop    bp                      
  0263e  50                 push   ax                      
  0263f  1e                 push   ds                      
  02640  babf21             mov    dx, 0x21bf              
  02643  0e                 push   cs                      
  02644  1f                 pop    ds                      
  02645  b024               mov    al, 0x24                
  02647  b425               mov    ah, 0x25                
  02649  cd21               int    0x21   ; DOS int21h AH=25 : set int vector
  0264b  1f                 pop    ds                      
  0264c  58                 pop    ax                      
  0264d  c3                 ret                            
  0264e  55                 push   bp                      
  0264f  8bec               mov    bp, sp                  
  02651  8b5e04             mov    bx, word ptr [bp + 4]   
  02654  8b5606             mov    dx, word ptr [bp + 6]   
  02657  8b4e08             mov    cx, word ptr [bp + 8]   
  0265a  b43f               mov    ah, 0x3f                
  0265c  cd21               int    0x21   ; DOS int21h AH=3f : READ file (BX,CX,DS:DX)
  0265e  7236               jb     0x2696                  
  02660  3d0f27             cmp    ax, 0x270f              
  02663  7431               je     0x2696                  
  02665  5d                 pop    bp                      
  02666  c3                 ret                            
  02667  55                 push   bp                      
  02668  8bec               mov    bp, sp                  
  0266a  8b5e04             mov    bx, word ptr [bp + 4]   
  0266d  8b5606             mov    dx, word ptr [bp + 6]   
  02670  8b4e08             mov    cx, word ptr [bp + 8]   
  02673  b440               mov    ah, 0x40                
  02675  cd21               int    0x21   ; DOS int21h AH=40 : WRITE file (BX,CX,DS:DX)
  02677  3b4608             cmp    ax, word ptr [bp + 8]   
  0267a  751a               jne    0x2696                  
  0267c  5d                 pop    bp                      
  0267d  c3                 ret                            
  0267e  55                 push   bp                      
  0267f  8bec               mov    bp, sp                  
  02681  8b5604             mov    dx, word ptr [bp + 4]   
  02684  b441               mov    ah, 0x41                
  02686  cd21               int    0x21   ; DOS int21h AH=41 : delete file
  02688  5d                 pop    bp                      
  02689  c3                 ret                            
  0268a  55                 push   bp                      
  0268b  8bec               mov    bp, sp                  
  0268d  8b5e04             mov    bx, word ptr [bp + 4]   
  02690  b43e               mov    ah, 0x3e                
  02692  cd21               int    0x21   ; DOS int21h AH=3e : close handle (BX)
  02694  5d                 pop    bp                      
  02695  c3                 ret                            
  02696  e873fc             call   0x230c                  
  02699  e8e6fb             call   0x2282                  
  0269c  e8b0fc             call   0x234f                  
  0269f  33c0               xor    ax, ax                  
  026a1  50                 push   ax                      
  026a2  e83100             call   0x26d6                  
  026a5  e80400             call   0x26ac                  
  026a8  86e0               xchg   al, ah                  
  026aa  ab                 stosw  word ptr es:[di], ax    
  026ab  c3                 ret                            
  026ac  53                 push   bx                      
  026ad  8d1eb905           lea    bx, [0x5b9]             
  026b1  8ae0               mov    ah, al                  
  026b3  d0e8               shr    al, 1                   
  026b5  d0e8               shr    al, 1                   
  026b7  d0e8               shr    al, 1                   
  026b9  d0e8               shr    al, 1                   
  026bb  d7                 xlatb                          
  026bc  86e0               xchg   al, ah                  
  026be  240f               and    al, 0xf                 
  026c0  d7                 xlatb                          
  026c1  5b                 pop    bx                      
  026c2  c3                 ret                            
  026c3  8e065909           mov    es, word ptr [0x959]    
  026c7  d1e2               shl    dx, 1                   
  026c9  d1e7               shl    di, 1                   
  026cb  d1e7               shl    di, 1                   
  026cd  d1e7               shl    di, 1                   
  026cf  d1e7               shl    di, 1                   
  026d1  8dbd6a09           lea    di, [di + 0x96a]        
  026d5  c3                 ret                            
  026d6  55                 push   bp                      
  026d7  8bec               mov    bp, sp                  
  026d9  8b4604             mov    ax, word ptr [bp + 4]   
  026dc  8b1e0303           mov    bx, word ptr [0x303]    
  026e0  0adb               or     bl, bl                  
  026e2  7428               je     0x270c                  
  026e4  7804               js     0x26ea                  
  026e6  0bc0               or     ax, ax                  
  026e8  7422               je     0x270c                  
  026ea  50                 push   ax                      
  026eb  8b5602             mov    dx, word ptr [bp + 2]   
  026ee  8d1e0303           lea    bx, [0x303]             
  026f2  e82900             call   0x271e                  
  026f5  8d160103           lea    dx, [0x301]             
  026f9  b409               mov    ah, 9                   
  026fb  cd21               int    0x21   ; DOS int21h AH=09 : print $-string (DS:DX)
  026fd  0bed               or     bp, bp                  
  026ff  740a               je     0x270b                  
  02701  396e00             cmp    word ptr [bp], bp       
  02704  7605               jbe    0x270b                  
  02706  8b6e00             mov    bp, word ptr [bp]       
  02709  ebe0               jmp    0x26eb                  
  0270b  58                 pop    ax                      
  0270c  8b1e0a03           mov    bx, word ptr [0x30a]    
  02710  0adb               or     bl, bl                  
  02712  7404               je     0x2718                  
  02714  b44c               mov    ah, 0x4c                
  02716  cd21               int    0x21   ; DOS int21h AH=4c : terminate w/code
  02718  8d361803           lea    si, [0x318]             
  0271c  ff2c               ljmp   [si]                    
  0271e  be0400             mov    si, 4                   
  02721  8ac2               mov    al, dl                  
  02723  b104               mov    cl, 4                   
  02725  d3ea               shr    dx, cl                  
  02727  240f               and    al, 0xf                 
  02729  0490               add    al, 0x90                
  0272b  27                 daa                            
  0272c  1440               adc    al, 0x40                
  0272e  27                 daa                            
  0272f  4e                 dec    si                      
  02730  8800               mov    byte ptr [bx + si], al  
  02732  75ed               jne    0x2721                  
  02734  c3                 ret                            
  02735  ba0e03             mov    dx, 0x30e               
  02738  b409               mov    ah, 9                   
  0273a  cd21               int    0x21   ; DOS int21h AH=09 : print $-string (DS:DX)
  0273c  b8ffff             mov    ax, 0xffff              
  0273f  50                 push   ax                      
  02740  e893ff             call   0x26d6                  
  02743  dbe3               fninit                         
  02745  b8c608             mov    ax, 0x8c6   ;  {reloc: seg fixup}
  02748  8ed8               mov    ds, ax                  
  0274a  8c061a03           mov    word ptr [0x31a], es    
  0274e  26a10200           mov    ax, word ptr es:[2]     
  02752  3b063a0b           cmp    ax, word ptr [0xb3a]    
  02756  76dd               jbe    0x2735                  
  02758  2b063a0b           sub    ax, word ptr [0xb3a]    
  0275c  8cdf               mov    di, ds                  
  0275e  81c70010           add    di, 0x1000              
  02762  3bf8               cmp    di, ax                  
  02764  7302               jae    0x2768                  
  02766  8bc7               mov    ax, di                  
  02768  be8013             mov    si, 0x1380   ;  {reloc: seg fixup}
  0276b  3bf0               cmp    si, ax                  
  0276d  73c6               jae    0x2735                  
  0276f  2bc6               sub    ax, si                  
  02771  3b06380b           cmp    ax, word ptr [0xb38]    
  02775  72be               jb     0x2735                  
  02777  3b06360b           cmp    ax, word ptr [0xb36]    
  0277b  7603               jbe    0x2780                  
  0277d  a1360b             mov    ax, word ptr [0xb36]    
  02780  03f0               add    si, ax                  
  02782  8cdf               mov    di, ds                  
  02784  8bc6               mov    ax, si                  
  02786  2bc7               sub    ax, di                  
  02788  3d0010             cmp    ax, 0x1000              
  0278b  7607               jbe    0x2794                  
  0278d  b80010             mov    ax, 0x1000              
  02790  2bf0               sub    si, ax                  
  02792  8bfe               mov    di, si                  
  02794  8bf0               mov    si, ax                  
  02796  b104               mov    cl, 4                   
  02798  d3e0               shl    ax, cl                  
  0279a  9c                 pushf                          
  0279b  59                 pop    cx                      
  0279c  fa                 cli                            
  0279d  8ed7               mov    ss, di                  
  0279f  8be0               mov    sp, ax                  
  027a1  33ed               xor    bp, bp                  
  027a3  55                 push   bp                      
  027a4  8bec               mov    bp, sp                  
  027a6  51                 push   cx                      
  027a7  9d                 popf                           
  027a8  06                 push   es                      
  027a9  03fe               add    di, si                  
  027ab  57                 push   di                      
  027ac  b430               mov    ah, 0x30                
  027ae  cd21               int    0x21   ; DOS int21h AH=30 : get DOS version
  027b0  0ac0               or     al, al                  
  027b2  7503               jne    0x27b7                  
  027b4  32e4               xor    ah, ah                  
  027b6  5b                 pop    bx                      
  027b7  a30a03             mov    word ptr [0x30a], ax    
  027ba  740b               je     0x27c7                  
  027bc  5b                 pop    bx                      
  027bd  58                 pop    ax                      
  027be  8ec0               mov    es, ax                  
  027c0  2bd8               sub    bx, ax                  
  027c2  06                 push   es                      
  027c3  b44a               mov    ah, 0x4a                
  027c5  cd21               int    0x21   ; DOS int21h AH=4a : resize mem
  027c7  1e                 push   ds                      
  027c8  07                 pop    es                      
  027c9  1f                 pop    ds                      
  027ca  be8000             mov    si, 0x80                
  027cd  8a0c               mov    cl, byte ptr [si]       
  027cf  80c103             add    cl, 3                   
  027d2  81e1fe00           and    cx, 0xfe                
  027d6  2be1               sub    sp, cx                  
  027d8  8bfc               mov    di, sp                  
  027da  fc                 cld                            
  027db  f3a4               rep movsb byte ptr es:[di], byte ptr [si]
  027dd  8bc4               mov    ax, sp                  
  027df  50                 push   ax                      
  027e0  06                 push   es                      
  027e1  1f                 pop    ds                      
  027e2  1e                 push   ds                      
  027e3  07                 pop    es                      
  027e4  bf3e0b             mov    di, 0xb3e               
  027e7  b9a7ab             mov    cx, 0xaba7              
  027ea  2bcf               sub    cx, di                  
  027ec  33c0               xor    ax, ax                  
  027ee  f3aa               rep stosb byte ptr es:[di], al    
  027f0  e81200             call   0x2805                  
  027f3  50                 push   ax                      
  027f4  e8dffe             call   0x26d6                  
  027f7  55                 push   bp                      
  027f8  8bec               mov    bp, sp                  
  027fa  8b5604             mov    dx, word ptr [bp + 4]   
  027fd  8b4606             mov    ax, word ptr [bp + 6]   
  02800  b400               mov    ah, 0                   
  02802  ee                 out    dx, al                  
  02803  5d                 pop    bp                      
  02804  c3                 ret                            
  02805  55                 push   bp                      
  02806  8bec               mov    bp, sp                  
  02808  83ec0e             sub    sp, 0xe                 
  0280b  8b4604             mov    ax, word ptr [bp + 4]   
  0280e  8946f2             mov    word ptr [bp - 0xe], ax 
  02811  8b76f2             mov    si, word ptr [bp - 0xe] 
  02814  ff46f2             inc    word ptr [bp - 0xe]     
  02817  8a04               mov    al, byte ptr [si]       
  02819  25ff00             and    ax, 0xff                
  0281c  8946fa             mov    word ptr [bp - 6], ax   
  0281f  8b46fa             mov    ax, word ptr [bp - 6]   
  02822  ff4efa             dec    word ptr [bp - 6]       
  02825  0bc0               or     ax, ax                  
  02827  7424               je     0x284d                  
  02829  8b76f2             mov    si, word ptr [bp - 0xe] 
  0282c  8a04               mov    al, byte ptr [si]       
  0282e  30e4               xor    ah, ah                  
  02830  83f820             cmp    ax, 0x20                
  02833  740c               je     0x2841                  
  02835  8b76f2             mov    si, word ptr [bp - 0xe] 
  02838  8a04               mov    al, byte ptr [si]       
  0283a  30e4               xor    ah, ah                  
  0283c  83f809             cmp    ax, 9                   
  0283f  7507               jne    0x2848                  
  02841  33c0               xor    ax, ax                  
  02843  8b76f2             mov    si, word ptr [bp - 0xe] 
  02846  8804               mov    byte ptr [si], al       
  02848  ff46f2             inc    word ptr [bp - 0xe]     
  0284b  ebd2               jmp    0x281f                  
  0284d  33c0               xor    ax, ax                  
  0284f  8b76f2             mov    si, word ptr [bp - 0xe] 
  02852  8804               mov    byte ptr [si], al       
  02854  8b4604             mov    ax, word ptr [bp + 4]   
  02857  8946f2             mov    word ptr [bp - 0xe], ax 
  0285a  b80100             mov    ax, 1                   
  0285d  8946fe             mov    word ptr [bp - 2], ax   
  02860  8b76f2             mov    si, word ptr [bp - 0xe] 
  02863  ff46f2             inc    word ptr [bp - 0xe]     
  02866  8a04               mov    al, byte ptr [si]       
  02868  25ff00             and    ax, 0xff                
  0286b  40                 inc    ax                      
  0286c  8946fa             mov    word ptr [bp - 6], ax   
  0286f  8b46fa             mov    ax, word ptr [bp - 6]   
  02872  ff4efa             dec    word ptr [bp - 6]       
  02875  0bc0               or     ax, ax                  
  02877  7425               je     0x289e                  
  02879  8b76f2             mov    si, word ptr [bp - 0xe] 
  0287c  8a04               mov    al, byte ptr [si]       
  0287e  25ff00             and    ax, 0xff                
  02881  7502               jne    0x2885                  
  02883  eb14               jmp    0x2899                  
  02885  8b46f2             mov    ax, word ptr [bp - 0xe] 
  02888  8d76f4             lea    si, [bp - 0xc]          
  0288b  8b56fe             mov    dx, word ptr [bp - 2]   
  0288e  ff46fe             inc    word ptr [bp - 2]       
  02891  d1e2               shl    dx, 1                   
  02893  03f2               add    si, dx                  
  02895  8904               mov    word ptr [si], ax       
  02897  eb05               jmp    0x289e                  
  02899  ff46f2             inc    word ptr [bp - 0xe]     
  0289c  ebd1               jmp    0x286f                  
  0289e  8d46f4             lea    ax, [bp - 0xc]          
  028a1  50                 push   ax                      
  028a2  ff76fe             push   word ptr [bp - 2]       
  028a5  e858d7             call   0                       
  028a8  5f                 pop    di                      
  028a9  5f                 pop    di                      
  028aa  50                 push   ax                      
  028ab  e80400             call   0x28b2                  
  028ae  8be5               mov    sp, bp                  
  028b0  5d                 pop    bp                      
  028b1  c3                 ret                            
  028b2  55                 push   bp                      
  028b3  8bec               mov    bp, sp                  
  028b5  ff7604             push   word ptr [bp + 4]       
  028b8  e81bfe             call   0x26d6                  
  028bb  83c402             add    sp, 2                   
  028be  8be5               mov    sp, bp                  
  028c0  5d                 pop    bp                      
  028c1  c3                 ret                            
  028c2  0000               add    byte ptr [bx + si], al  
  028c4  0000               add    byte ptr [bx + si], al  
  028c6  0000               add    byte ptr [bx + si], al  
  028c8  0000               add    byte ptr [bx + si], al  
  028ca  0000               add    byte ptr [bx + si], al  
  028cc  0000               add    byte ptr [bx + si], al  
  028ce  0000               add    byte ptr [bx + si], al  

;==============================================================================
; ROOT CODE  (frame 0x028D): C STARTUP, OVERLAY LOADER, ROOT GAME CODE
; Entry point c_startup_entry (028D:02FA / image 0x2BCA) sets up stack & DS,
; then runs main. overlay_loader_vector (028D:02D0) + overlay_loader_main
; (028D:0287) implement the Phoenix overlay paging described in the header.
;==============================================================================
  028d0  be0000             mov    si, 0                   
  028d3  268b8c0400         mov    cx, word ptr es:[si + 4]
  028d8  41                 inc    cx                      
  028d9  742e               je     0x2909                  
  028db  268b940800         mov    dx, word ptr es:[si + 8]
  028e0  268b850800         mov    ax, word ptr es:[di + 8]
  028e5  50                 push   ax                      
  028e6  2bc2               sub    ax, dx                  
  028e8  58                 pop    ax                      
  028e9  7309               jae    0x28f4                  
  028eb  8bc2               mov    ax, dx                  
  028ed  268b950a00         mov    dx, word ptr es:[di + 0xa]
  028f2  eb05               jmp    0x28f9                  
  028f4  268b940a00         mov    dx, word ptr es:[si + 0xa]
  028f9  2bc2               sub    ax, dx                  
  028fb  7307               jae    0x2904                  
  028fd  2681a40400ff7f     and    word ptr es:[si + 4], 0x7fff
  02904  83c610             add    si, 0x10                
  02907  ebca               jmp    0x28d3                  
  02909  c3                 ret                            
  0290a  8bec               mov    bp, sp                  
  0290c  c7460effff         mov    word ptr [bp + 0xe], 0xffff
  02911  b000               mov    al, 0                   
  02913  cf                 iret                           
  02914  8b1e2600           mov    bx, word ptr [0x26]     
  02918  0ac0               or     al, al                  
  0291a  7508               jne    0x2924                  
  0291c  263b9d0c00         cmp    bx, word ptr es:[di + 0xc]
  02921  7501               jne    0x2924                  
  02923  c3                 ret                            
  02924  50                 push   ax                      
  02925  0bdb               or     bx, bx                  
  02927  740f               je     0x2938                  
  02929  f6060e00ff         test   byte ptr [0xe], 0xff    
  0292e  7408               je     0x2938                  
  02930  8b1e1000           mov    bx, word ptr [0x10]     
  02934  b43e               mov    ah, 0x3e                
  02936  cd21               int    0x21   ; DOS int21h AH=3e : close handle (BX)
  02938  58                 pop    ax                      
  02939  268b9d0c00         mov    bx, word ptr es:[di + 0xc]
  0293e  891e2600           mov    word ptr [0x26], bx     
  02942  50                 push   ax                      
  02943  8b362600           mov    si, word ptr [0x26]     
  02947  bf5b00             mov    di, 0x5b                
  0294a  26ac               lodsb  al, byte ptr es:[si]    
  0294c  8805               mov    byte ptr [di], al       
  0294e  47                 inc    di                      
  0294f  0ac0               or     al, al                  
  02951  75f7               jne    0x294a                  
  02953  58                 pop    ax                      
  02954  06                 push   es                      
  02955  1e                 push   ds                      
  02956  07                 pop    es                      
  02957  e85304             call   0x2dad                  
  0295a  07                 pop    es                      
  0295b  a8ff               test   al, 0xff                
  0295d  7402               je     0x2961                  
  0295f  f9                 stc                            
  02960  c3                 ret                            
  02961  f6060e00ff         test   byte ptr [0xe], 0xff    
  02966  7527               jne    0x298f                  
  02968  be5b00             mov    si, 0x5b                
  0296b  bf2900             mov    di, 0x29                
  0296e  06                 push   es                      
  0296f  8cd8               mov    ax, ds                  
  02971  8ec0               mov    es, ax                  
  02973  32c0               xor    al, al                  
  02975  b429               mov    ah, 0x29                
  02977  cd21               int    0x21   ; DOS int21h AH=29 : ?
  02979  07                 pop    es                      
  0297a  33f6               xor    si, si                  
  0297c  b40f               mov    ah, 0xf                 
  0297e  ba2900             mov    dx, 0x29                
  02981  cd21               int    0x21   ; DOS int21h AH=0f : ?
  02983  0ac0               or     al, al                  
  02985  751a               jne    0x29a1                  
  02987  c70637000100       mov    word ptr [0x37], 1      
  0298d  f8                 clc                            
  0298e  c3                 ret                            
  0298f  a00f00             mov    al, byte ptr [0xf]      
  02992  33f6               xor    si, si                  
  02994  b43d               mov    ah, 0x3d                
  02996  ba5b00             mov    dx, 0x5b                
  02999  cd21               int    0x21   ; DOS int21h AH=3d : OPEN FILE (DS:DX name,AL mode)
  0299b  7204               jb     0x29a1                  
  0299d  a31000             mov    word ptr [0x10], ax     
  029a0  c3                 ret                            
  029a1  b001               mov    al, 1                   
  029a3  0bf6               or     si, si                  
  029a5  7402               je     0x29a9                  
  029a7  fec0               inc    al                      
  029a9  eb97               jmp    0x2942                  
  029ab  268b9d0e00         mov    bx, word ptr es:[di + 0xe]
  029b0  03da               add    bx, dx                  
  029b2  268b851000         mov    ax, word ptr es:[di + 0x10]
  029b7  150000             adc    ax, 0                   
  029ba  b90400             mov    cx, 4                   
  029bd  d1e3               shl    bx, 1                   
  029bf  d1d0               rcl    ax, 1                   
  029c1  e2fa               loop   0x29bd                  
  029c3  f6060e00ff         test   byte ptr [0xe], 0xff    
  029c8  7508               jne    0x29d2                  
  029ca  891e4a00           mov    word ptr [0x4a], bx     
  029ce  a34c00             mov    word ptr [0x4c], ax     
  029d1  c3                 ret                            
  029d2  8bc8               mov    cx, ax                  
  029d4  8bd3               mov    dx, bx                  
  029d6  8b1e1000           mov    bx, word ptr [0x10]     
  029da  b000               mov    al, 0                   
  029dc  b442               mov    ah, 0x42                
  029de  cd21               int    0x21   ; DOS int21h AH=42 : LSEEK (BX,CX:DX,AL)
  029e0  c3                 ret                            
  029e1  50                 push   ax                      
  029e2  52                 push   dx                      
  029e3  b81000             mov    ax, 0x10                
  029e6  f7e1               mul    cx                      
  029e8  b9f0ff             mov    cx, 0xfff0              
  029eb  f7f1               div    cx                      
  029ed  8bca               mov    cx, dx                  
  029ef  5a                 pop    dx                      
  029f0  5b                 pop    bx                      
  029f1  a9ffff             test   ax, 0xffff              
  029f4  750c               jne    0x2a02                  
  029f6  f7c1ffff           test   cx, 0xffff              
  029fa  7502               jne    0x29fe                  
  029fc  f8                 clc                            
  029fd  c3                 ret                            
  029fe  50                 push   ax                      
  029ff  50                 push   ax                      
  02a00  eb06               jmp    0x2a08                  
  02a02  48                 dec    ax                      
  02a03  50                 push   ax                      
  02a04  51                 push   cx                      
  02a05  b9f0ff             mov    cx, 0xfff0              
  02a08  52                 push   dx                      
  02a09  53                 push   bx                      
  02a0a  f6060e00ff         test   byte ptr [0xe], 0xff    
  02a0f  7517               jne    0x2a28                  
  02a11  1e                 push   ds                      
  02a12  8edb               mov    ds, bx                  
  02a14  b41a               mov    ah, 0x1a                
  02a16  cd21               int    0x21   ; DOS int21h AH=1a : set DTA
  02a18  1f                 pop    ds                      
  02a19  33f6               xor    si, si                  
  02a1b  b427               mov    ah, 0x27                
  02a1d  ba2900             mov    dx, 0x29                
  02a20  cd21               int    0x21   ; DOS int21h AH=27 : ?
  02a22  0ac0               or     al, al                  
  02a24  7519               jne    0x2a3f                  
  02a26  eb1a               jmp    0x2a42                  
  02a28  a11000             mov    ax, word ptr [0x10]     
  02a2b  51                 push   cx                      
  02a2c  1e                 push   ds                      
  02a2d  8edb               mov    ds, bx                  
  02a2f  8bd8               mov    bx, ax                  
  02a31  33f6               xor    si, si                  
  02a33  b43f               mov    ah, 0x3f                
  02a35  cd21               int    0x21   ; DOS int21h AH=3f : READ file (BX,CX,DS:DX)
  02a37  1f                 pop    ds                      
  02a38  59                 pop    cx                      
  02a39  7204               jb     0x2a3f                  
  02a3b  3bc1               cmp    ax, cx                  
  02a3d  7403               je     0x2a42                  
  02a3f  f9                 stc                            
  02a40  eb01               jmp    0x2a43                  
  02a42  f8                 clc                            
  02a43  5b                 pop    bx                      
  02a44  5a                 pop    dx                      
  02a45  59                 pop    cx                      
  02a46  58                 pop    ax                      
  02a47  7206               jb     0x2a4f                  
  02a49  81c3ff0f           add    bx, 0xfff               
  02a4d  eba2               jmp    0x29f1                  
  02a4f  b003               mov    al, 3                   
  02a51  0bf6               or     si, si                  
  02a53  7402               je     0x2a57                  
  02a55  fec0               inc    al                      
  02a57  f9                 stc                            
  02a58  c3                 ret                            
  02a59  43                 inc    bx                      
  02a5a  6f                 outsw  dx, word ptr [si]       
  02a5b  7079               jo     0x2ad6                  
  02a5d  7269               jb     0x2ac8                  
  02a5f  67687420           push   0x2074                  
  02a63  284329             sub    byte ptr [bp + di + 0x29], al
  02a66  2031               and    byte ptr [bx + di], dh  
  02a68  3938               cmp    word ptr [bx + si], di  
  02a6a  3320               xor    sp, word ptr [bx + si]  
  02a6c  627920             bound  di, dword ptr [bx + di + 0x20]
  02a6f  50                 push   ax                      
  02a70  686f65             push   0x656f                  
  02a73  6e                 outsb  dx, byte ptr [si]       
  02a74  697820536f         imul   di, word ptr [bx + si + 0x20], 0x6f53
  02a79  667477             je     0x2af3                  
  02a7c  61                 popaw                          
  02a7d  7265               jb     0x2ae4                  
  02a7f  204173             and    byte ptr [bx + di + 0x73], al
  02a82  736f               jae    0x2af3                  
  02a84  636961             arpl   word ptr [bx + di + 0x61], bp
  02a87  7465               je     0x2aee                  
  02a89  7320               jae    0x2aab                  
  02a8b  4c                 dec    sp                      
  02a8c  7464               je     0x2af2                  
  02a8e  2ec5362200         lds    si, ptr cs:[0x22]       
  02a93  ff34               push   word ptr [si]           
  02a95  ff7402             push   word ptr [si + 2]       
  02a98  c7043a00           mov    word ptr [si], 0x3a     
  02a9c  8c4c02             mov    word ptr [si + 2], cs   
  02a9f  b8de02             mov    ax, 0x2de   ;  {reloc: seg fixup}
  02aa2  8ed8               mov    ds, ax                  
  02aa4  b000               mov    al, 0                   
  02aa6  57                 push   di                      
  02aa7  e86afe             call   0x2914                  
  02aaa  5f                 pop    di                      
  02aab  7303               jae    0x2ab0                  
  02aad  e99400             jmp    0x2b44                  
  02ab0  268b950600         mov    dx, word ptr es:[di + 6]
  02ab5  83c203             add    dx, 3                   
  02ab8  d1ea               shr    dx, 1                   
  02aba  d1ea               shr    dx, 1                   
  02abc  e8ecfe             call   0x29ab                  
  02abf  268b8d1200         mov    cx, word ptr es:[di + 0x12]
  02ac4  268b850a00         mov    ax, word ptr es:[di + 0xa]
  02ac9  262b850800         sub    ax, word ptr es:[di + 8]
  02ace  3bc1               cmp    ax, cx                  
  02ad0  7302               jae    0x2ad4                  
  02ad2  8bc8               mov    cx, ax                  
  02ad4  268b850800         mov    ax, word ptr es:[di + 8]
  02ad9  ba0000             mov    dx, 0                   
  02adc  e802ff             call   0x29e1                  
  02adf  72c5               jb     0x2aa6                  
  02ae1  ba0000             mov    dx, 0                   
  02ae4  e8c4fe             call   0x29ab                  
  02ae7  268b8d0600         mov    cx, word ptr es:[di + 6]
  02aec  bb0000             mov    bx, 0                   
  02aef  268b160000         mov    dx, word ptr es:[0]     
  02af4  06                 push   es                      
  02af5  f7c1ffff           test   cx, 0xffff              
  02af9  7441               je     0x2b3c                  
  02afb  f7c3ffff           test   bx, 0xffff              
  02aff  7517               jne    0x2b18                  
  02b01  51                 push   cx                      
  02b02  52                 push   dx                      
  02b03  8cd8               mov    ax, ds                  
  02b05  ba1200             mov    dx, 0x12                
  02b08  b90100             mov    cx, 1                   
  02b0b  e8d3fe             call   0x29e1                  
  02b0e  7296               jb     0x2aa6                  
  02b10  5a                 pop    dx                      
  02b11  59                 pop    cx                      
  02b12  bb0400             mov    bx, 4                   
  02b15  be0000             mov    si, 0                   
  02b18  49                 dec    cx                      
  02b19  4b                 dec    bx                      
  02b1a  8b841200           mov    ax, word ptr [si + 0x12]
  02b1e  8be8               mov    bp, ax                  
  02b20  8b841400           mov    ax, word ptr [si + 0x14]
  02b24  f7c5ffff           test   bp, 0xffff              
  02b28  7505               jne    0x2b2f                  
  02b2a  a9ffff             test   ax, 0xffff              
  02b2d  7408               je     0x2b37                  
  02b2f  03c2               add    ax, dx                  
  02b31  8ec0               mov    es, ax                  
  02b33  26015600           add    word ptr es:[bp], dx    
  02b37  83c604             add    si, 4                   
  02b3a  ebb9               jmp    0x2af5                  
  02b3c  07                 pop    es                      
  02b3d  26818d04000080     or     word ptr es:[di + 4], 0x8000
  02b44  c5362200           lds    si, ptr [0x22]          
  02b48  8f4402             pop    word ptr [si + 2]       
  02b4b  8f04               pop    word ptr [si]           
  02b4d  b8de02             mov    ax, 0x2de   ;  {reloc: seg fixup}
  02b50  8ed8               mov    ds, ax                  
  02b52  c3                 ret                            
  02b53  0000               add    byte ptr [bx + si], al  
  02b55  0000               add    byte ptr [bx + si], al  

overlay_loader_main:
  02b57  fc                 cld                            
  02b58  1e                 push   ds                      
  02b59  06                 push   es                      
  02b5a  bbde02             mov    bx, 0x2de   ;  {reloc: seg fixup}
  02b5d  8edb               mov    ds, bx                  
  02b5f  bbf602             mov    bx, 0x2f6   ;  {reloc: seg fixup}
  02b62  8ec3               mov    es, bx                  
  02b64  88262800           mov    byte ptr [0x28], ah     
  02b68  b81000             mov    ax, 0x10                
  02b6b  49                 dec    cx                      
  02b6c  f7e1               mul    cx                      
  02b6e  8bf8               mov    di, ax                  
  02b70  26f78504000080     test   word ptr es:[di + 4], 0x8000
  02b77  750a               jne    0x2b83                  
  02b79  e854fd             call   0x28d0                  
  02b7c  e810ff             call   0x2a8f                  
  02b7f  720f               jb     0x2b90                  
  02b81  eb00               jmp    0x2b83                  
  02b83  268b8d0400         mov    cx, word ptr es:[di + 4]
  02b88  81e1ff3f           and    cx, 0x3fff              
  02b8c  75da               jne    0x2b68                  
  02b8e  eb0d               jmp    0x2b9d                  
  02b90  f6062800ff         test   byte ptr [0x28], 0xff   
  02b95  7505               jne    0x2b9c                  
  02b97  2eff2e8302         ljmp   cs:[0x283]              
  02b9c  f9                 stc                            
  02b9d  07                 pop    es                      
  02b9e  1f                 pop    ds                      
  02b9f  cb                 retf                           

overlay_loader_vector:
  02ba0  9c                 pushf                          
  02ba1  50                 push   ax                      
  02ba2  53                 push   bx                      
  02ba3  51                 push   cx                      
  02ba4  52                 push   dx                      
  02ba5  55                 push   bp                      
  02ba6  56                 push   si                      
  02ba7  57                 push   di                      
  02ba8  8bec               mov    bp, sp                  
  02baa  8b4612             mov    ax, word ptr [bp + 0x12]
  02bad  1e                 push   ds                      
  02bae  8ed8               mov    ds, ax                  
  02bb0  8b7610             mov    si, word ptr [bp + 0x10]
  02bb3  83461002           add    word ptr [bp + 0x10], 2 
  02bb7  8b0c               mov    cx, word ptr [si]       
  02bb9  1f                 pop    ds                      
  02bba  b400               mov    ah, 0                   
  02bbc  9a87028d02         lcall  0x28d, 0x287   ; --> overlay_loader_main {reloc: seg fixup}
  02bc1  5f                 pop    di                      
  02bc2  5e                 pop    si                      
  02bc3  5d                 pop    bp                      
  02bc4  5a                 pop    dx                      
  02bc5  59                 pop    cx                      
  02bc6  5b                 pop    bx                      
  02bc7  58                 pop    ax                      
  02bc8  9d                 popf                           
  02bc9  cb                 retf                           

c_startup_entry:
  02bca  fa                 cli                            
  02bcb  8cc7               mov    di, es                  
  02bcd  8cd6               mov    si, ss                  
  02bcf  8bcc               mov    cx, sp                  
  02bd1  baf602             mov    dx, 0x2f6   ;  {reloc: seg fixup}
  02bd4  8ec2               mov    es, dx                  
  02bd6  268e160200         mov    ss, word ptr es:[2]     
  02bdb  bc0001             mov    sp, 0x100               
  02bde  fb                 sti                            
  02bdf  8ec7               mov    es, di                  
  02be1  56                 push   si                      
  02be2  51                 push   cx                      
  02be3  50                 push   ax                      
  02be4  1e                 push   ds                      
  02be5  06                 push   es                      
  02be6  2e8c068502         mov    word ptr cs:[0x285], es 
  02beb  b8de02             mov    ax, 0x2de   ;  {reloc: seg fixup}
  02bee  8ed8               mov    ds, ax                  
  02bf0  e8aa00             call   0x2c9d                  
  02bf3  b8f602             mov    ax, 0x2f6   ;  {reloc: seg fixup}
  02bf6  8ec0               mov    es, ax                  
  02bf8  8cc8               mov    ax, cs                  
  02bfa  262b060000         sub    ax, word ptr es:[0]     
  02bff  26a30000           mov    word ptr es:[0], ax     
  02c03  b430               mov    ah, 0x30                
  02c05  cd21               int    0x21   ; DOS int21h AH=30 : get DOS version
  02c07  b300               mov    bl, 0                   
  02c09  3c02               cmp    al, 2                   
  02c0b  7219               jb     0x2c26                  
  02c0d  bd8a00             mov    bp, 0x8a                
  02c10  bb0000             mov    bx, 0                   
  02c13  b4ef               mov    ah, 0xef                
  02c15  cd21               int    0x21   ; DOS int21h AH=ef : ?
  02c17  b000               mov    al, 0                   
  02c19  f7c30100           test   bx, 1                   
  02c1d  7402               je     0x2c21                  
  02c1f  b030               mov    al, 0x30                
  02c21  a20f00             mov    byte ptr [0xf], al      
  02c24  b3ff               mov    bl, 0xff                
  02c26  881e0e00           mov    byte ptr [0xe], bl      
  02c2a  bf0000             mov    di, 0                   
  02c2d  b90100             mov    cx, 1                   
  02c30  268b850400         mov    ax, word ptr es:[di + 4]
  02c35  40                 inc    ax                      
  02c36  7418               je     0x2c50                  
  02c38  26f78504000040     test   word ptr es:[di + 4], 0x4000
  02c3f  7409               je     0x2c4a                  
  02c41  57                 push   di                      
  02c42  51                 push   cx                      
  02c43  9a87028d02         lcall  0x28d, 0x287   ; --> overlay_loader_main {reloc: seg fixup}
  02c48  59                 pop    cx                      
  02c49  5f                 pop    di                      
  02c4a  41                 inc    cx                      
  02c4b  83c710             add    di, 0x10                
  02c4e  ebe0               jmp    0x2c30                  
  02c50  07                 pop    es                      
  02c51  1f                 pop    ds                      
  02c52  58                 pop    ax                      
  02c53  59                 pop    cx                      
  02c54  5b                 pop    bx                      
  02c55  fa                 cli                            
  02c56  8be1               mov    sp, cx                  
  02c58  8ed3               mov    ss, bx                  
  02c5a  fb                 sti                            
  02c5b  ea43270000         ljmp   0:0x2743   ;  {reloc: seg fixup}
  02c60  b020               mov    al, 0x20                
  02c62  ae                 scasb  al, byte ptr es:[di]    
  02c63  74fd               je     0x2c62                  
  02c65  4f                 dec    di                      
  02c66  c3                 ret                            
  02c67  26a12c00           mov    ax, word ptr es:[0x2c]  
  02c6b  a34f00             mov    word ptr [0x4f], ax     
  02c6e  8ec0               mov    es, ax                  
  02c70  bf0000             mov    di, 0                   
  02c73  fc                 cld                            
  02c74  26f605ff           test   byte ptr es:[di], 0xff  
  02c78  7422               je     0x2c9c                  
  02c7a  be5500             mov    si, 0x55                
  02c7d  b90400             mov    cx, 4                   
  02c80  f3a6               repe cmpsb byte ptr [si], byte ptr es:[di]
  02c82  7407               je     0x2c8b                  
  02c84  32c0               xor    al, al                  
  02c86  ae                 scasb  al, byte ptr es:[di]    
  02c87  75fd               jne    0x2c86                  
  02c89  ebe9               jmp    0x2c74                  
  02c8b  e8d2ff             call   0x2c60                  
  02c8e  26803d3d           cmp    byte ptr es:[di], 0x3d  
  02c92  75f0               jne    0x2c84                  
  02c94  47                 inc    di                      
  02c95  e8c8ff             call   0x2c60                  
  02c98  893e5100           mov    word ptr [0x51], di     
  02c9c  c3                 ret                            
  02c9d  b430               mov    ah, 0x30                
  02c9f  cd21               int    0x21   ; DOS int21h AH=30 : get DOS version
  02ca1  b300               mov    bl, 0                   
  02ca3  3c02               cmp    al, 2                   
  02ca5  7205               jb     0x2cac                  
  02ca7  e8bdff             call   0x2c67                  
  02caa  b3ff               mov    bl, 0xff                
  02cac  881e4e00           mov    byte ptr [0x4e], bl     
  02cb0  c3                 ret                            
  02cb1  f6065a00ff         test   byte ptr [0x5a], 0xff   
  02cb6  7406               je     0x2cbe                  
  02cb8  bead00             mov    si, 0xad                
  02cbb  e80300             call   0x2cc1                  
  02cbe  be5b00             mov    si, 0x5b                
  02cc1  8a14               mov    dl, byte ptr [si]       
  02cc3  0ad2               or     dl, dl                  
  02cc5  7409               je     0x2cd0                  
  02cc7  56                 push   si                      
  02cc8  b402               mov    ah, 2                   
  02cca  cd21               int    0x21   ; DOS int21h AH=02 : display char (DL)
  02ccc  5e                 pop    si                      
  02ccd  46                 inc    si                      
  02cce  ebf1               jmp    0x2cc1                  
  02cd0  c3                 ret                            
  02cd1  b409               mov    ah, 9                   
  02cd3  ba0b01             mov    dx, 0x10b               
  02cd6  cd21               int    0x21   ; DOS int21h AH=09 : print $-string (DS:DX)
  02cd8  b40a               mov    ah, 0xa                 
  02cda  baab00             mov    dx, 0xab                
  02cdd  cd21               int    0x21   ; DOS int21h AH=0a : buffered kbd input
  02cdf  b409               mov    ah, 9                   
  02ce1  ba7c01             mov    dx, 0x17c               
  02ce4  cd21               int    0x21   ; DOS int21h AH=09 : print $-string (DS:DX)
  02ce6  a0ad00             mov    al, byte ptr [0xad]     
  02ce9  3c2e               cmp    al, 0x2e                
  02ceb  7429               je     0x2d16                  
  02ced  8a1eac00           mov    bl, byte ptr [0xac]     
  02cf1  32ff               xor    bh, bh                  
  02cf3  c687ad0000         mov    byte ptr [bx + 0xad], 0 
  02cf8  be0000             mov    si, 0                   
  02cfb  8a84ad00           mov    al, byte ptr [si + 0xad]
  02cff  a8ff               test   al, 0xff                
  02d01  7411               je     0x2d14                  
  02d03  3c7b               cmp    al, 0x7b                
  02d05  730a               jae    0x2d11                  
  02d07  3c61               cmp    al, 0x61                
  02d09  7206               jb     0x2d11                  
  02d0b  24df               and    al, 0xdf                
  02d0d  8884ad00           mov    byte ptr [si + 0xad], al
  02d11  46                 inc    si                      
  02d12  ebe7               jmp    0x2cfb                  
  02d14  f8                 clc                            
  02d15  c3                 ret                            
  02d16  f9                 stc                            
  02d17  c3                 ret                            
  02d18  8cc3               mov    bx, es                  
  02d1a  8edd               mov    ds, bp                  
  02d1c  56                 push   si                      
  02d1d  4a                 dec    dx                      
  02d1e  ac                 lodsb  al, byte ptr [si]       
  02d1f  42                 inc    dx                      
  02d20  0ac0               or     al, al                  
  02d22  7408               je     0x2d2c                  
  02d24  3c20               cmp    al, 0x20                
  02d26  7404               je     0x2d2c                  
  02d28  3c3b               cmp    al, 0x3b                
  02d2a  75f2               jne    0x2d1e                  
  02d2c  be5b00             mov    si, 0x5b                
  02d2f  8edb               mov    ds, bx                  
  02d31  56                 push   si                      
  02d32  83c64f             add    si, 0x4f                
  02d35  8bfe               mov    di, si                  
  02d37  2bf2               sub    si, dx                  
  02d39  b95000             mov    cx, 0x50                
  02d3c  2bca               sub    cx, dx                  
  02d3e  fd                 std                            
  02d3f  f3a4               rep movsb byte ptr es:[di], byte ptr [si]
  02d41  fc                 cld                            
  02d42  5f                 pop    di                      
  02d43  5e                 pop    si                      
  02d44  8edd               mov    ds, bp                  
  02d46  8ec3               mov    es, bx                  
  02d48  8bca               mov    cx, dx                  
  02d4a  f3a4               rep movsb byte ptr es:[di], byte ptr [si]
  02d4c  8edb               mov    ds, bx                  
  02d4e  c3                 ret                            
  02d4f  8cdd               mov    bp, ds                  
  02d51  bead00             mov    si, 0xad                
  02d54  ba0000             mov    dx, 0                   
  02d57  ebbf               jmp    0x2d18                  
  02d59  f6064e00ff         test   byte ptr [0x4e], 0xff   
  02d5e  7430               je     0x2d90                  
  02d60  8b365300           mov    si, word ptr [0x53]     
  02d64  0bf6               or     si, si                  
  02d66  7428               je     0x2d90                  
  02d68  1e                 push   ds                      
  02d69  a14f00             mov    ax, word ptr [0x4f]     
  02d6c  8ed8               mov    ds, ax                  
  02d6e  ac                 lodsb  al, byte ptr [si]       
  02d6f  3c3b               cmp    al, 0x3b                
  02d71  74fb               je     0x2d6e                  
  02d73  3c20               cmp    al, 0x20                
  02d75  74f7               je     0x2d6e                  
  02d77  4e                 dec    si                      
  02d78  f604ff             test   byte ptr [si], 0xff     
  02d7b  8cdd               mov    bp, ds                  
  02d7d  1f                 pop    ds                      
  02d7e  7410               je     0x2d90                  
  02d80  ba0100             mov    dx, 1                   
  02d83  e892ff             call   0x2d18                  
  02d86  89365300           mov    word ptr [0x53], si     
  02d8a  4f                 dec    di                      
  02d8b  c6055c             mov    byte ptr [di], 0x5c     
  02d8e  f8                 clc                            
  02d8f  c3                 ret                            
  02d90  f9                 stc                            
  02d91  c3                 ret                            
  02d92  50                 push   ax                      
  02d93  b409               mov    ah, 9                   
  02d95  baef00             mov    dx, 0xef                
  02d98  cd21               int    0x21   ; DOS int21h AH=09 : print $-string (DS:DX)
  02d9a  58                 pop    ax                      
  02d9b  ba5801             mov    dx, 0x158               
  02d9e  3c03               cmp    al, 3                   
  02da0  7203               jb     0x2da5                  
  02da2  ba6901             mov    dx, 0x169               
  02da5  b409               mov    ah, 9                   
  02da7  cd21               int    0x21   ; DOS int21h AH=09 : print $-string (DS:DX)
  02da9  e805ff             call   0x2cb1                  
  02dac  c3                 ret                            
  02dad  fc                 cld                            
  02dae  0ac0               or     al, al                  
  02db0  7514               jne    0x2dc6                  
  02db2  c606590000         mov    byte ptr [0x59], 0      
  02db7  c6065a0000         mov    byte ptr [0x5a], 0      
  02dbc  8b1e5100           mov    bx, word ptr [0x51]     
  02dc0  891e5300           mov    word ptr [0x53], bx     
  02dc4  eb22               jmp    0x2de8                  
  02dc6  f6065900ff         test   byte ptr [0x59], 0xff   
  02dcb  750b               jne    0x2dd8                  
  02dcd  e889ff             call   0x2d59                  
  02dd0  7316               jae    0x2de8                  
  02dd2  fe065900           inc    byte ptr [0x59]         
  02dd6  eb0d               jmp    0x2de5                  
  02dd8  e8b7ff             call   0x2d92                  
  02ddb  c6065a0001         mov    byte ptr [0x5a], 1      
  02de0  e8eefe             call   0x2cd1                  
  02de3  7206               jb     0x2deb                  
  02de5  e867ff             call   0x2d4f                  
  02de8  b000               mov    al, 0                   
  02dea  c3                 ret                            
  02deb  b001               mov    al, 1                   
  02ded  c3                 ret                            
  02dee  0000               add    byte ptr [bx + si], al  
  02df0  0000               add    byte ptr [bx + si], al  
  02df2  0000               add    byte ptr [bx + si], al  
  02df4  0000               add    byte ptr [bx + si], al  
  02df6  0000               add    byte ptr [bx + si], al  
  02df8  0000               add    byte ptr [bx + si], al  
  02dfa  0000               add    byte ptr [bx + si], al  
  02dfc  0000               add    byte ptr [bx + si], al  
  02dfe  0000               add    byte ptr [bx + si], al  
  02e00  0000               add    byte ptr [bx + si], al  
  02e02  90                 nop                            
  02e03  0000               add    byte ptr [bx + si], al  
  02e05  0000               add    byte ptr [bx + si], al  
  02e07  0000               add    byte ptr [bx + si], al  
  02e09  0020               add    byte ptr [bx + si], ah  
  02e0b  2020               and    byte ptr [bx + si], ah  
  02e0d  2020               and    byte ptr [bx + si], ah  
  02e0f  2020               and    byte ptr [bx + si], ah  
  02e11  2020               and    byte ptr [bx + si], ah  
  02e13  2020               and    byte ptr [bx + si], ah  
  02e15  0000               add    byte ptr [bx + si], al  
  02e17  0000               add    byte ptr [bx + si], al  
  02e19  0000               add    byte ptr [bx + si], al  
  02e1b  0000               add    byte ptr [bx + si], al  
  02e1d  0000               add    byte ptr [bx + si], al  
  02e1f  0000               add    byte ptr [bx + si], al  
  02e21  0000               add    byte ptr [bx + si], al  
  02e23  0000               add    byte ptr [bx + si], al  
  02e25  0000               add    byte ptr [bx + si], al  
  02e27  0000               add    byte ptr [bx + si], al  
  02e29  0000               add    byte ptr [bx + si], al  
  02e2b  0000               add    byte ptr [bx + si], al  
  02e2d  0000               add    byte ptr [bx + si], al  
  02e2f  0000               add    byte ptr [bx + si], al  
  02e31  0000               add    byte ptr [bx + si], al  
  02e33  0000               add    byte ptr [bx + si], al  
  02e35  50                 push   ax                      
  02e36  41                 inc    cx                      
  02e37  54                 push   sp                      
  02e38  48                 dec    ax                      
  02e39  0000               add    byte ptr [bx + si], al  
  02e3b  0000               add    byte ptr [bx + si], al  
  02e3d  0000               add    byte ptr [bx + si], al  
  02e3f  0000               add    byte ptr [bx + si], al  
  02e41  0000               add    byte ptr [bx + si], al  
  02e43  0000               add    byte ptr [bx + si], al  
  02e45  0000               add    byte ptr [bx + si], al  
  02e47  0000               add    byte ptr [bx + si], al  
  02e49  0000               add    byte ptr [bx + si], al  
  02e4b  0000               add    byte ptr [bx + si], al  
  02e4d  0000               add    byte ptr [bx + si], al  
  02e4f  0000               add    byte ptr [bx + si], al  
  02e51  0000               add    byte ptr [bx + si], al  
  02e53  0000               add    byte ptr [bx + si], al  
  02e55  0000               add    byte ptr [bx + si], al  
  02e57  0000               add    byte ptr [bx + si], al  
  02e59  0000               add    byte ptr [bx + si], al  
  02e5b  0000               add    byte ptr [bx + si], al  
  02e5d  0000               add    byte ptr [bx + si], al  
  02e5f  0000               add    byte ptr [bx + si], al  
  02e61  0000               add    byte ptr [bx + si], al  
  02e63  0000               add    byte ptr [bx + si], al  
  02e65  0000               add    byte ptr [bx + si], al  
  02e67  0000               add    byte ptr [bx + si], al  
  02e69  0000               add    byte ptr [bx + si], al  
  02e6b  0000               add    byte ptr [bx + si], al  
  02e6d  0000               add    byte ptr [bx + si], al  
  02e6f  0000               add    byte ptr [bx + si], al  
  02e71  0000               add    byte ptr [bx + si], al  
  02e73  0000               add    byte ptr [bx + si], al  
  02e75  0000               add    byte ptr [bx + si], al  
  02e77  0000               add    byte ptr [bx + si], al  
  02e79  0000               add    byte ptr [bx + si], al  
  02e7b  0000               add    byte ptr [bx + si], al  
  02e7d  0000               add    byte ptr [bx + si], al  
  02e7f  0000               add    byte ptr [bx + si], al  
  02e81  0000               add    byte ptr [bx + si], al  
  02e83  0000               add    byte ptr [bx + si], al  
  02e85  0000               add    byte ptr [bx + si], al  
  02e87  0000               add    byte ptr [bx + si], al  
  02e89  0000               add    byte ptr [bx + si], al  
  02e8b  42                 inc    dx                      
  02e8c  00413a             add    byte ptr [bx + di + 0x3a], al
  02e8f  0000               add    byte ptr [bx + si], al  
  02e91  0000               add    byte ptr [bx + si], al  
  02e93  0000               add    byte ptr [bx + si], al  
  02e95  0000               add    byte ptr [bx + si], al  
  02e97  0000               add    byte ptr [bx + si], al  
  02e99  0000               add    byte ptr [bx + si], al  
  02e9b  0000               add    byte ptr [bx + si], al  
  02e9d  0000               add    byte ptr [bx + si], al  
  02e9f  0000               add    byte ptr [bx + si], al  
  02ea1  0000               add    byte ptr [bx + si], al  
  02ea3  0000               add    byte ptr [bx + si], al  
  02ea5  0000               add    byte ptr [bx + si], al  
  02ea7  0000               add    byte ptr [bx + si], al  
  02ea9  0000               add    byte ptr [bx + si], al  
  02eab  0000               add    byte ptr [bx + si], al  
  02ead  0000               add    byte ptr [bx + si], al  
  02eaf  0000               add    byte ptr [bx + si], al  
  02eb1  0000               add    byte ptr [bx + si], al  
  02eb3  0000               add    byte ptr [bx + si], al  
  02eb5  0000               add    byte ptr [bx + si], al  
  02eb7  0000               add    byte ptr [bx + si], al  
  02eb9  0000               add    byte ptr [bx + si], al  
  02ebb  0000               add    byte ptr [bx + si], al  
  02ebd  0000               add    byte ptr [bx + si], al  
  02ebf  0000               add    byte ptr [bx + si], al  
  02ec1  0000               add    byte ptr [bx + si], al  
  02ec3  0000               add    byte ptr [bx + si], al  
  02ec5  0000               add    byte ptr [bx + si], al  
  02ec7  0000               add    byte ptr [bx + si], al  
  02ec9  0000               add    byte ptr [bx + si], al  
  02ecb  0000               add    byte ptr [bx + si], al  
  02ecd  0000               add    byte ptr [bx + si], al  

;==============================================================================
; ROOT DATA: loader strings, overlay descriptor table, thunk table
;==============================================================================
  02ed1  db "$LINK86 Overlay Loader - $."
  02eee  db "INSERT WAR DISK AND PRESS ENTER$th name/) or '.' to quit=>$Fatal error - $Can't find file $Disk I/O error in $"
  ; --- overlay descriptor table (raw words) ---
  02f60  dw 028Dh, 1391h, 0000h, 0000h, 030Fh, 0852h, 0086h, 032Fh
  02f70  dw 0000h, 0543h, 0001h, 0000h, 0852h, 08B0h, 0086h, 0872h
  02f80  dw 0000h, 005Eh, 0000h, 0000h, 030Fh, 04BEh, 0086h, 08D0h
  02f90  dw 0000h, 01AFh, 0000h, 0000h, 030Fh, 03BCh, 0086h, 0A7Fh
  02fa0  dw 0000h, 00ADh, 0000h, 0000h, 030Fh, 0478h, 0086h, 0B2Ch
  02fb0  dw 0000h, 0169h, 0000h, 0000h, 030Fh, 08C6h, 0086h, 0C95h
  02fc0  dw 0000h, 05B7h, 0000h, 0000h, 030Fh, 07F1h, 0086h, 124Ch
  02fd0  dw 0000h, 04E2h, 4000h, 0000h, 08C6h, 1391h, 0086h, 172Eh
  02fe0  dw 0000h, 00B4h, FFFFh, 4157h, 2E52h, 5845h, 0045h, D09Ah
  02fe6  db "WAR.EXE",0
  ; --- overlay call thunks (see table in header) ---
  02fee  lcall 028D:02D0 / dw 1 / jmp far 30F0:seg   ; thunk 0 -> ovl1_fn_30f0 (file 0x032F0)
  02ffa  lcall 028D:02D0 / dw 1 / jmp far 325F:seg   ; thunk 1 -> ovl1_fn_325f (file 0x0345F)
  03006  lcall 028D:02D0 / dw 6 / jmp far 30F0:seg   ; thunk 2 -> ovl6_fn_30f0 (file 0x0C950)
  03012  lcall 028D:02D0 / dw 7 / jmp far 30F0:seg   ; thunk 3 -> ovl7_fn_30f0 (file 0x124C0)
  0301e  lcall 028D:02D0 / dw 1 / jmp far 35C4:seg   ; thunk 4 -> ovl1_fn_35c4 (file 0x037C4)
  0302a  lcall 028D:02D0 / dw 1 / jmp far 53AE:seg   ; thunk 5 -> ovl1_fn_53ae (file 0x055AE)
  03036  lcall 028D:02D0 / dw 1 / jmp far 3D1E:seg   ; thunk 6 -> ovl1_fn_3d1e (file 0x03F1E)
  03042  lcall 028D:02D0 / dw 1 / jmp far 4EB2:seg   ; thunk 7 -> ovl1_fn_4eb2 (file 0x050B2)
  0304e  lcall 028D:02D0 / dw 4 / jmp far 30F0:seg   ; thunk 8 -> ovl4_fn_30f0 (file 0x0A7F0)
  0305a  lcall 028D:02D0 / dw 3 / jmp far 30F0:seg   ; thunk 9 -> ovl3_fn_30f0 (file 0x08D00)
  03066  lcall 028D:02D0 / dw 1 / jmp far 3E08:seg   ; thunk 10 -> ovl1_fn_3e08 (file 0x04008)
  03072  lcall 028D:02D0 / dw 1 / jmp far 4000:seg   ; thunk 11 -> ovl1_fn_4000 (file 0x04200)
  0307e  lcall 028D:02D0 / dw 5 / jmp far 30F0:seg   ; thunk 12 -> ovl5_fn_30f0 (file 0x0B2C0)
  0308a  lcall 028D:02D0 / dw 1 / jmp far 5782:seg   ; thunk 13 -> ovl1_fn_5782 (file 0x05982)
  03096  lcall 028D:02D0 / dw 1 / jmp far 39AA:seg   ; thunk 14 -> ovl1_fn_39aa (file 0x03BAA)
  030a2  lcall 028D:02D0 / dw 1 / jmp far 389B:seg   ; thunk 15 -> ovl1_fn_389b (file 0x03A9B)
  030ae  lcall 028D:02D0 / dw 1 / jmp far 3EB6:seg   ; thunk 16 -> ovl1_fn_3eb6 (file 0x040B6)
  030ba  lcall 028D:02D0 / dw 1 / jmp far 3FC3:seg   ; thunk 17 -> ovl1_fn_3fc3 (file 0x041C3)
  030c6  lcall 028D:02D0 / dw 1 / jmp far 4808:seg   ; thunk 18 -> ovl1_fn_4808 (file 0x04A08)
  030d2  lcall 028D:02D0 / dw 1 / jmp far 7EDA:seg   ; thunk 19 -> ovl1_fn_7eda (file 0x080DA)
  030de  lcall 028D:02D0 / dw 2 / jmp far 8520:seg   ; thunk 20 -> ovl2_fn_8520 (file 0x08720)

;##############################################################################
; OVERLAY CODE - entry-point windows for each overlaid routine
; (Overlays share one runtime segment; near calls to image < 0x30F0 reach
;  resident root/library code. Overlay bodies also contain packed graphics
;  data which is NOT valid code - only entry regions are disassembled.)
;##############################################################################

;==============================================================================
; OVERLAY 1 entry ovl1_fn_30f0   (file 0x032F0, loads to seg-offset 0x30F0)
;==============================================================================

ovl1_fn_30f0:
  030f0  55                 push   bp                      
  030f1  8bec               mov    bp, sp                  
  030f3  e82df2             call   0x2323                  
  030f6  b80100             mov    ax, 1                   
  030f9  8906dfa6           mov    word ptr [0xa6df], ax   
  030fd  b02d               mov    al, 0x2d                
  030ff  8806dea6           mov    byte ptr [0xa6de], al   
  03103  e8d44d             call   0x7eda   ; -> ovl1_fn_7eda
  03106  e8f746             call   0x7800                  
  03109  0bc0               or     ax, ax                  
  0310b  7507               jne    0x3114                  
  0310d  b80f27             mov    ax, 0x270f              
  03110  8be5               mov    sp, bp                  
  03112  5d                 pop    bp                      
  03113  c3                 ret                            
  03114  b89001             mov    ax, 0x190               
  03117  8906e3a6           mov    word ptr [0xa6e3], ax   
  0311b  b80200             mov    ax, 2                   
  0311e  8906e1a6           mov    word ptr [0xa6e1], ax   
  03122  8906dca6           mov    word ptr [0xa6dc], ax   
  03126  33c0               xor    ax, ax                  
  03128  8906f5a6           mov    word ptr [0xa6f5], ax   
  0312c  b019               mov    al, 0x19                
  0312e  50                 push   ax                      
  0312f  bedea6             mov    si, 0xa6de              
  03132  56                 push   si                      
  03133  b84500             mov    ax, 0x45                
  03136  50                 push   ax                      
  03137  e8b3e6             call   0x17ed                  
  0313a  83c406             add    sp, 6                   
  0313d  b80200             mov    ax, 2                   
  03140  50                 push   ax                      
  03141  b8d403             mov    ax, 0x3d4               
  03144  50                 push   ax                      
  03145  e8aff6             call   0x27f7                  
  03148  5f                 pop    di                      
  03149  5f                 pop    di                      
  0314a  8a06dea6           mov    al, byte ptr [0xa6de]   
  0314e  98                 cwde                           
  0314f  50                 push   ax                      
  03150  b8d503             mov    ax, 0x3d5               
  03153  50                 push   ax                      
  03154  e8a0f6             call   0x27f7                  
  03157  5f                 pop    di                      
  03158  5f                 pop    di                      
  03159  e8b851             call   0x8314                  
  0315c  09c0               or     ax, ax                  
  0315e  7506               jne    0x3166                  
  03160  33c0               xor    ax, ax                  
  03162  8906e7a6           mov    word ptr [0xa6e7], ax   
  03166  e82def             call   0x2096                  
  03169  813e45050000       cmp    word ptr [0x545], 0     
  0316f  7506               jne    0x3177                  
  03171  33c0               xor    ax, ax                  
  03173  8906f5a6           mov    word ptr [0xa6f5], ax   
  03177  e864ff             call   0x30de                  
  0317a  b8680b             mov    ax, 0xb68               
  0317d  50                 push   ax                      
  0317e  b84900             mov    ax, 0x49                
  03181  50                 push   ax                      
  03182  e8de4f             call   0x8163                  
  03185  5f                 pop    di                      
  03186  5f                 pop    di                      
  03187  09c0               or     ax, ax                  
  03189  7507               jne    0x3192                  
  0318b  b80f27             mov    ax, 0x270f              
  0318e  8be5               mov    sp, bp                  
  03190  5d                 pop    bp                      
  03191  c3                 ret                            
  03192  b8c800             mov    ax, 0xc8                
  03195  50                 push   ax                      
  03196  e810e6             call   0x17a9                  
  03199  5f                 pop    di                      
  0319a  b80f00             mov    ax, 0xf                 
  0319d  50                 push   ax                      
  0319e  b00c               mov    al, 0xc                 
  031a0  50                 push   ax                      
  031a1  b00b               mov    al, 0xb                 
  031a3  50                 push   ax                      
  031a4  33c0               xor    ax, ax                  
  031a6  50                 push   ax                      
  031a7  b00f               mov    al, 0xf                 
  031a9  50                 push   ax                      
  031aa  b00b               mov    al, 0xb                 
  031ac  50                 push   ax                      
  031ad  b007               mov    al, 7                   
  031af  50                 push   ax                      
  031b0  33c0               xor    ax, ax                  
  031b2  50                 push   ax                      
  031b3  b00f               mov    al, 0xf                 
  031b5  50                 push   ax                      
  031b6  b00d               mov    al, 0xd                 
  031b8  50                 push   ax                      
  031b9  b003               mov    al, 3                   
  031bb  50                 push   ax                      
  031bc  33c0               xor    ax, ax                  
  031be  50                 push   ax                      
  031bf  e844d4             call   0x606                   
  031c2  83c418             add    sp, 0x18                
  031c5  b80c00             mov    ax, 0xc                 
  031c8  50                 push   ax                      
  031c9  b00d               mov    al, 0xd                 
  031cb  50                 push   ax                      
  031cc  e88dee             call   0x205c                  
  031cf  5f                 pop    di                      
  031d0  5f                 pop    di                      
  031d1  b84c00             mov    ax, 0x4c                
  031d4  50                 push   ax                      
  031d5  e88df3             call   0x2565                  
  031d8  5f                 pop    di                      
  031d9  e8b803             call   0x3594                  
  031dc  0bc0               or     ax, ax                  
  031de  7407               je     0x31e7                  
  031e0  b80f27             mov    ax, 0x270f              
  031e3  8be5               mov    sp, bp                  
  031e5  5d                 pop    bp                      
  031e6  c3                 ret                            
  031e7  813ee1a60300       cmp    word ptr [0xa6e1], 3    
  031ed  750f               jne    0x31fe                  
  031ef  813e41050000       cmp    word ptr [0x541], 0     
  031f5  7507               jne    0x31fe                  
  031f7  b80200             mov    ax, 2                   
  031fa  8906e1a6           mov    word ptr [0xa6e1], ax   
  031fe  813ee1a60200       cmp    word ptr [0xa6e1], 2    
  03204  7517               jne    0x321d                  
  03206  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0320a  058b04             add    ax, 0x48b               
  0320d  50                 push   ax                      
  0320e  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03212  054704             add    ax, 0x447               
  03215  50                 push   ax                      
  03216  e81314             call   0x462c                  
  03219  5f                 pop    di                      
  0321a  5f                 pop    di                      
  0321b  eb3c               jmp    0x3259                  
  0321d  813ee1a60300       cmp    word ptr [0xa6e1], 3    
  03223  7528               jne    0x324d                  
  03225  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03229  05e711             add    ax, 0x11e7              
  0322c  50                 push   ax                      
  0322d  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03231  059f11             add    ax, 0x119f              
  03234  50                 push   ax                      
  03235  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03239  055711             add    ax, 0x1157              
  0323c  50                 push   ax                      
  0323d  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03241  050d11             add    ax, 0x110d              
  03244  50                 push   ax                      
  03245  e85714             call   0x469f                  
  03248  83c408             add    sp, 8                   
  0324b  eb0c               jmp    0x3259                  
  0324d  b860ea             mov    ax, 0xea60              
  03250  33d2               xor    dx, dx                  
  03252  52                 push   dx                      
  03253  50                 push   ax                      
  03254  e831ef             call   0x2188                  
  03257  5f                 pop    di                      
  03258  5f                 pop    di                      
  03259  33c0               xor    ax, ax                  
  0325b  8be5               mov    sp, bp                  
  0325d  5d                 pop    bp                      
  0325e  c3                 ret                            

ovl1_fn_325f:
  0325f  55                 push   bp                      
  03260  8bec               mov    bp, sp                  
  03262  83ec08             sub    sp, 8                   
  03265  b8680b             mov    ax, 0xb68               
  03268  89066888           mov    word ptr [0x8868], ax   
  0326c  33c0               xor    ax, ax                  

;==============================================================================
; OVERLAY 1 entry ovl1_fn_325f   (file 0x0345F, loads to seg-offset 0x325F)
;==============================================================================

ovl1_fn_325f:
  0325f  55                 push   bp                      
  03260  8bec               mov    bp, sp                  
  03262  83ec08             sub    sp, 8                   
  03265  b8680b             mov    ax, 0xb68               
  03268  89066888           mov    word ptr [0x8868], ax   
  0326c  33c0               xor    ax, ax                  
  0326e  8906cea5           mov    word ptr [0xa5ce], ax   
  03272  890680a6           mov    word ptr [0xa680], ax   
  03276  89067ea6           mov    word ptr [0xa67e], ax   
  0327a  890618a5           mov    word ptr [0xa518], ax   
  0327e  8946fe             mov    word ptr [bp - 2], ax   
  03281  8906d8a6           mov    word ptr [0xa6d8], ax   
  03285  b002               mov    al, 2                   
  03287  8906dca6           mov    word ptr [0xa6dc], ax   
  0328b  e8d902             call   0x3567                  
  0328e  0bc0               or     ax, ax                  
  03290  7407               je     0x3299                  
  03292  b80f27             mov    ax, 0x270f              
  03295  8be5               mov    sp, bp                  
  03297  5d                 pop    bp                      
  03298  c3                 ret                            
  03299  e8c50e             call   0x4161                  
  0329c  8946f8             mov    word ptr [bp - 8], ax   
  0329f  83f864             cmp    ax, 0x64                
  032a2  7506               jne    0x32aa                  
  032a4  33c0               xor    ax, ax                  
  032a6  8be5               mov    sp, bp                  
  032a8  5d                 pop    bp                      
  032a9  c3                 ret                            
  032aa  8b46f8             mov    ax, word ptr [bp - 8]   
  032ad  0bc0               or     ax, ax                  
  032af  7407               je     0x32b8                  
  032b1  8b46f8             mov    ax, word ptr [bp - 8]   
  032b4  8be5               mov    sp, bp                  
  032b6  5d                 pop    bp                      
  032b7  c3                 ret                            
  032b8  b8782d             mov    ax, 0x2d78              
  032bb  89066888           mov    word ptr [0x8868], ax   
  032bf  b86d00             mov    ax, 0x6d                
  032c2  89066a88           mov    word ptr [0x886a], ax   
  032c6  b026               mov    al, 0x26                
  032c8  89064a9e           mov    word ptr [0x9e4a], ax   
  032cc  b091               mov    al, 0x91                
  032ce  89064c9e           mov    word ptr [0x9e4c], ax   
  032d2  b8de38             mov    ax, 0x38de              
  032d5  89064e9e           mov    word ptr [0x9e4e], ax   
  032d9  b8de38             mov    ax, 0x38de              
  032dc  89064e9e           mov    word ptr [0x9e4e], ax   
  032e0  33c0               xor    ax, ax                  
  032e2  8946f8             mov    word ptr [bp - 8], ax   
  032e5  817ef81400         cmp    word ptr [bp - 8], 0x14 
  032ea  7c03               jl     0x32ef                  
  032ec  e98100             jmp    0x3370                  
  032ef  b80f27             mov    ax, 0x270f              
  032f2  be1ea5             mov    si, 0xa51e              
  032f5  8b56f8             mov    dx, word ptr [bp - 8]   
  032f8  d1e2               shl    dx, 1                   
  032fa  03f2               add    si, dx                  
  032fc  8904               mov    word ptr [si], ax       
  032fe  b8ffff             mov    ax, 0xffff              
  03301  be2ca0             mov    si, 0xa02c              
  03304  8b56f8             mov    dx, word ptr [bp - 8]   
  03307  d1e2               shl    dx, 1                   
  03309  03f2               add    si, dx                  
  0330b  8904               mov    word ptr [si], ax       
  0330d  b8feff             mov    ax, 0xfffe              
  03310  be04a0             mov    si, 0xa004              
  03313  8b56f8             mov    dx, word ptr [bp - 8]   
  03316  d1e2               shl    dx, 1                   
  03318  03f2               add    si, dx                  
  0331a  8904               mov    word ptr [si], ax       
  0331c  813e0fa75000       cmp    word ptr [0xa70f], 0x50 
  03322  7c22               jl     0x3346                  
  03324  bedc9f             mov    si, 0x9fdc              
  03327  0376f8             add    si, word ptr [bp - 8]   
  0332a  8a04               mov    al, byte ptr [si]       
  0332c  98                 cwde                           
  0332d  bec89f             mov    si, 0x9fc8              
  03330  0376f8             add    si, word ptr [bp - 8]   
  03333  50                 push   ax                      
  03334  5a                 pop    dx                      
  03335  8a04               mov    al, byte ptr [si]       
  03337  98                 cwde                           
  03338  03c2               add    ax, dx                  
  0333a  8804               mov    byte ptr [si], al       
  0333c  33c0               xor    ax, ax                  
  0333e  bedc9f             mov    si, 0x9fdc              
  03341  0376f8             add    si, word ptr [bp - 8]   
  03344  8804               mov    byte ptr [si], al       
  03346  be609e             mov    si, 0x9e60              
  03349  8b46f8             mov    ax, word ptr [bp - 8]   
  0334c  d1e0               shl    ax, 1                   
  0334e  03f0               add    si, ax                  
  03350  8b04               mov    ax, word ptr [si]       
  03352  09c0               or     ax, ax                  
  03354  7514               jne    0x336a                  
  03356  beb09e             mov    si, 0x9eb0              
  03359  8b46f8             mov    ax, word ptr [bp - 8]   
  0335c  d1e0               shl    ax, 1                   
  0335e  03f0               add    si, ax                  
  03360  8b04               mov    ax, word ptr [si]       
  03362  3d0f27             cmp    ax, 0x270f              
  03365  7403               je     0x336a                  
  03367  ff46fe             inc    word ptr [bp - 2]       
  0336a  ff46f8             inc    word ptr [bp - 8]       
  0336d  e975ff             jmp    0x32e5                  
  03370  b80100             mov    ax, 1                   
  03373  8b56fe             mov    dx, word ptr [bp - 2]   
  03376  8bc8               mov    cx, ax                  
  03378  d3fa               sar    dx, cl                  
  0337a  8956fe             mov    word ptr [bp - 2], dx   
  0337d  bee8a4             mov    si, 0xa4e8              
  03380  8b04               mov    ax, word ptr [si]       
  03382  3b46fe             cmp    ax, word ptr [bp - 2]   
  03385  7e08               jle    0x338f                  
  03387  8b46fe             mov    ax, word ptr [bp - 2]   
  0338a  bee8a4             mov    si, 0xa4e8              
  0338d  8904               mov    word ptr [si], ax       
  0338f  b85401             mov    ax, 0x154               
  03392  50                 push   ax                      
  03393  b8b8a0             mov    ax, 0xa0b8              
  03396  50                 push   ax                      
  03397  33c0               xor    ax, ax                  
  03399  50                 push   ax                      
  0339a  e878eb             call   0x1f15                  
  0339d  83c406             add    sp, 6                   
  033a0  b8b004             mov    ax, 0x4b0               
  033a3  50                 push   ax                      
  033a4  b88297             mov    ax, 0x9782              
  033a7  50                 push   ax                      
  033a8  33c0               xor    ax, ax                  
  033aa  50                 push   ax                      
  033ab  e867eb             call   0x1f15                  
  033ae  83c406             add    sp, 6                   
  033b1  b80900             mov    ax, 9                   
  033b4  50                 push   ax                      
  033b5  b85505             mov    ax, 0x555               
  033b8  50                 push   ax                      
  033b9  33c0               xor    ax, ax                  
  033bb  50                 push   ax                      
  033bc  e856eb             call   0x1f15                  
  033bf  83c406             add    sp, 6                   
  033c2  b80100             mov    ax, 1                   
  033c5  8906cca5           mov    word ptr [0xa5cc], ax   
  033c9  8b06d8a6           mov    ax, word ptr [0xa6d8]   
  033cd  0bc0               or     ax, ax                  
  033cf  7409               je     0x33da                  
  033d1  b80100             mov    ax, 1                   
  033d4  89061ca5           mov    word ptr [0xa51c], ax   
  033d8  eb07               jmp    0x33e1                  
  033da  b84000             mov    ax, 0x40                

;==============================================================================
; OVERLAY 1 entry ovl1_fn_35c4   (file 0x037C4, loads to seg-offset 0x35C4)
;==============================================================================

ovl1_fn_35c4:
  035c4  55                 push   bp                      
  035c5  8bec               mov    bp, sp                  
  035c7  83ec02             sub    sp, 2                   
  035ca  b80200             mov    ax, 2                   
  035cd  8906c0a5           mov    word ptr [0xa5c0], ax   
  035d1  33c0               xor    ax, ax                  
  035d3  50                 push   ax                      
  035d4  e884cf             call   0x55b                   
  035d7  5f                 pop    di                      
  035d8  e8c509             call   0x3fa0                  
  035db  813edca60200       cmp    word ptr [0xa6dc], 2    
  035e1  7506               jne    0x35e9                  
  035e3  33c0               xor    ax, ax                  
  035e5  8906dca6           mov    word ptr [0xa6dc], ax   
  035e9  e8a8ff             call   0x3594                  
  035ec  0bc0               or     ax, ax                  
  035ee  7414               je     0x3604                  
  035f0  33c0               xor    ax, ax                  
  035f2  50                 push   ax                      
  035f3  e80ace             call   0x400                   
  035f6  5f                 pop    di                      
  035f7  0bc0               or     ax, ax                  
  035f9  7407               je     0x3602                  
  035fb  b80100             mov    ax, 1                   
  035fe  8be5               mov    sp, bp                  
  03600  5d                 pop    bp                      
  03601  c3                 ret                            
  03602  ebe5               jmp    0x35e9                  
  03604  b8680b             mov    ax, 0xb68               
  03607  8906d0a6           mov    word ptr [0xa6d0], ax   
  0360b  b8160f             mov    ax, 0xf16               
  0360e  50                 push   ax                      
  0360f  ff36d0a6           push   word ptr [0xa6d0]       
  03613  b85f00             mov    ax, 0x5f                
  03616  50                 push   ax                      
  03617  e8d3e1             call   0x17ed                  
  0361a  83c406             add    sp, 6                   
  0361d  33c0               xor    ax, ax                  
  0361f  8946fe             mov    word ptr [bp - 2], ax   
  03622  e8bc02             call   0x38e1                  
  03625  813edca60200       cmp    word ptr [0xa6dc], 2    
  0362b  7508               jne    0x3635                  
  0362d  33c0               xor    ax, ax                  
  0362f  8906dca6           mov    word ptr [0xa6dc], ax   
  03633  ebed               jmp    0x3622                  
  03635  b86200             mov    ax, 0x62                
  03638  50                 push   ax                      
  03639  e8da41             call   0x7816                  
  0363c  5f                 pop    di                      
  0363d  e881d0             call   0x6c1                   
  03640  b80400             mov    ax, 4                   
  03643  50                 push   ax                      
  03644  b83e0b             mov    ax, 0xb3e               
  03647  50                 push   ax                      
  03648  e8bfef             call   0x260a                  
  0364b  5f                 pop    di                      
  0364c  5f                 pop    di                      
  0364d  8906aaa9           mov    word ptr [0xa9aa], ax   
  03651  813eaaa90f27       cmp    word ptr [0xa9aa], 0x270f
  03657  7503               jne    0x365c                  
  03659  e96901             jmp    0x37c5                  
  0365c  8b06aaa9           mov    ax, word ptr [0xa9aa]   
  03660  0bc0               or     ax, ax                  
  03662  7503               jne    0x3667                  
  03664  e98400             jmp    0x36eb                  
  03667  ff36aaa9           push   word ptr [0xa9aa]       
  0366b  e81cf0             call   0x268a                  
  0366e  5f                 pop    di                      
  0366f  b81500             mov    ax, 0x15                
  03672  50                 push   ax                      
  03673  33c0               xor    ax, ax                  
  03675  50                 push   ax                      
  03676  e8e3e9             call   0x205c                  
  03679  5f                 pop    di                      
  0367a  5f                 pop    di                      
  0367b  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0367f  05070f             add    ax, 0xf07               
  03682  50                 push   ax                      
  03683  e8dfee             call   0x2565                  
  03686  5f                 pop    di                      
  03687  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0368b  05300f             add    ax, 0xf30               
  0368e  50                 push   ax                      
  0368f  e8d3ee             call   0x2565                  
  03692  5f                 pop    di                      
  03693  b81800             mov    ax, 0x18                
  03696  50                 push   ax                      
  03697  e844e8             call   0x1ede                  
  0369a  5f                 pop    di                      
  0369b  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0369f  05590f             add    ax, 0xf59               
  036a2  50                 push   ax                      
  036a3  e8bfee             call   0x2565                  
  036a6  5f                 pop    di                      
  036a7  b80100             mov    ax, 1                   
  036aa  0bc0               or     ax, ax                  
  036ac  743d               je     0x36eb                  
  036ae  813e2f052e00       cmp    word ptr [0x52f], 0x2e  
  036b4  7522               jne    0x36d8                  
  036b6  b81500             mov    ax, 0x15                
  036b9  50                 push   ax                      
  036ba  e821e8             call   0x1ede                  
  036bd  5f                 pop    di                      
  036be  b81600             mov    ax, 0x16                
  036c1  50                 push   ax                      
  036c2  e819e8             call   0x1ede                  
  036c5  5f                 pop    di                      
  036c6  b81700             mov    ax, 0x17                
  036c9  50                 push   ax                      
  036ca  e811e8             call   0x1ede                  
  036cd  5f                 pop    di                      
  036ce  b81800             mov    ax, 0x18                
  036d1  50                 push   ax                      
  036d2  e809e8             call   0x1ede                  
  036d5  5f                 pop    di                      
  036d6  eb13               jmp    0x36eb                  
  036d8  813e2f051000       cmp    word ptr [0x52f], 0x10  
  036de  7509               jne    0x36e9                  
  036e0  e86cec             call   0x234f                  
  036e3  33c0               xor    ax, ax                  
  036e5  8be5               mov    sp, bp                  
  036e7  5d                 pop    bp                      
  036e8  c3                 ret                            
  036e9  ebbc               jmp    0x36a7                  
  036eb  b80500             mov    ax, 5                   
  036ee  50                 push   ax                      
  036ef  b83e0b             mov    ax, 0xb3e               
  036f2  50                 push   ax                      
  036f3  e814ef             call   0x260a                  
  036f6  5f                 pop    di                      
  036f7  5f                 pop    di                      
  036f8  8906aaa9           mov    word ptr [0xa9aa], ax   
  036fc  813eaaa90000       cmp    word ptr [0xa9aa], 0    
  03702  7536               jne    0x373a                  
  03704  ff46fe             inc    word ptr [bp - 2]       
  03707  8b46fe             mov    ax, word ptr [bp - 2]   
  0370a  83f801             cmp    ax, 1                   
  0370d  7e03               jle    0x3712                  
  0370f  e9b300             jmp    0x37c5                  
  03712  e80740             call   0x771c                  
  03715  0bc0               or     ax, ax                  
  03717  7418               je     0x3731                  
  03719  e88408             call   0x3fa0                  
  0371c  b80c00             mov    ax, 0xc                 
  0371f  50                 push   ax                      
  03720  50                 push   ax                      
  03721  e838e9             call   0x205c                  
  03724  5f                 pop    di                      
  03725  5f                 pop    di                      
  03726  b86400             mov    ax, 0x64                
  03729  50                 push   ax                      
  0372a  e838ee             call   0x2565                  
  0372d  5f                 pop    di                      
  0372e  e99400             jmp    0x37c5                  
  03731  33c0               xor    ax, ax                  
  03733  8906dca6           mov    word ptr [0xa6dc], ax   
  03737  e9e8fe             jmp    0x3622                  
  0373a  813eaaa90f27       cmp    word ptr [0xa9aa], 0x270f
  03740  7503               jne    0x3745                  

;==============================================================================
; OVERLAY 1 entry ovl1_fn_389b   (file 0x03A9B, loads to seg-offset 0x389B)
;==============================================================================

ovl1_fn_389b:
  0389b  55                 push   bp                      
  0389c  8bec               mov    bp, sp                  
  0389e  b8ffff             mov    ax, 0xffff              
  038a1  50                 push   ax                      
  038a2  e8a3c8             call   0x148                   
  038a5  5f                 pop    di                      
  038a6  0bc0               or     ax, ax                  
  038a8  7533               jne    0x38dd                  
  038aa  e8ca00             call   0x3977                  
  038ad  813ec0a50100       cmp    word ptr [0xa5c0], 1    
  038b3  7e08               jle    0x38bd                  
  038b5  b81700             mov    ax, 0x17                
  038b8  50                 push   ax                      
  038b9  e822e6             call   0x1ede                  
  038bc  5f                 pop    di                      
  038bd  e8193f             call   0x77d9                  
  038c0  33c0               xor    ax, ax                  
  038c2  8906dca6           mov    word ptr [0xa6dc], ax   
  038c6  b8ffff             mov    ax, 0xffff              
  038c9  50                 push   ax                      
  038ca  e87bc8             call   0x148                   
  038cd  5f                 pop    di                      
  038ce  09c0               or     ax, ax                  
  038d0  7505               jne    0x38d7                  
  038d2  e837ea             call   0x230c                  
  038d5  ebe6               jmp    0x38bd                  
  038d7  33c0               xor    ax, ax                  
  038d9  89063105           mov    word ptr [0x531], ax    
  038dd  8be5               mov    sp, bp                  
  038df  5d                 pop    bp                      
  038e0  c3                 ret                            
  038e1  55                 push   bp                      
  038e2  8bec               mov    bp, sp                  
  038e4  813edca60000       cmp    word ptr [0xa6dc], 0    
  038ea  7403               je     0x38ef                  
  038ec  e98400             jmp    0x3973                  
  038ef  813ec0a50100       cmp    word ptr [0xa5c0], 1    
  038f5  7e08               jle    0x38ff                  
  038f7  b81700             mov    ax, 0x17                
  038fa  50                 push   ax                      
  038fb  e8e0e5             call   0x1ede                  
  038fe  5f                 pop    di                      
  038ff  b81800             mov    ax, 0x18                
  03902  50                 push   ax                      
  03903  e8d8e5             call   0x1ede                  
  03906  5f                 pop    di                      
  03907  e86d00             call   0x3977                  
  0390a  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  0390e  05e70d             add    ax, 0xde7               
  03911  50                 push   ax                      
  03912  e850ec             call   0x2565                  
  03915  5f                 pop    di                      
  03916  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  0391a  05d70d             add    ax, 0xdd7               
  0391d  50                 push   ax                      
  0391e  e844ec             call   0x2565                  
  03921  5f                 pop    di                      
  03922  813ef5a60000       cmp    word ptr [0xa6f5], 0    
  03928  750a               jne    0x3934                  
  0392a  b84100             mov    ax, 0x41                
  0392d  50                 push   ax                      
  0392e  e8b6eb             call   0x24e7                  
  03931  5f                 pop    di                      
  03932  eb08               jmp    0x393c                  
  03934  b84200             mov    ax, 0x42                
  03937  50                 push   ax                      
  03938  e8aceb             call   0x24e7                  
  0393b  5f                 pop    di                      
  0393c  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  03940  05130e             add    ax, 0xe13               
  03943  50                 push   ax                      
  03944  e81eec             call   0x2565                  
  03947  5f                 pop    di                      
  03948  813e2f050000       cmp    word ptr [0x52f], 0     
  0394e  74f8               je     0x3948                  
  03950  b80100             mov    ax, 1                   
  03953  8906dca6           mov    word ptr [0xa6dc], ax   
  03957  ff36f5a6           push   word ptr [0xa6f5]       
  0395b  e8eac7             call   0x148                   
  0395e  5f                 pop    di                      
  0395f  0bc0               or     ax, ax                  
  03961  740a               je     0x396d                  
  03963  e8a6e9             call   0x230c                  
  03966  b80200             mov    ax, 2                   
  03969  8906dca6           mov    word ptr [0xa6dc], ax   
  0396d  33c0               xor    ax, ax                  
  0396f  89063105           mov    word ptr [0x531], ax    
  03973  8be5               mov    sp, bp                  
  03975  5d                 pop    bp                      
  03976  c3                 ret                            
  03977  55                 push   bp                      
  03978  8bec               mov    bp, sp                  
  0397a  83ec02             sub    sp, 2                   
  0397d  e86fe7             call   0x20ef                  
  03980  b8581b             mov    ax, 0x1b58              
  03983  8946fe             mov    word ptr [bp - 2], ax   
  03986  817efeb80b         cmp    word ptr [bp - 2], 0xbb8
  0398b  7e16               jle    0x39a3                  
  0398d  ff76fe             push   word ptr [bp - 2]       
  03990  e849e7             call   0x20dc                  
  03993  5f                 pop    di                      
  03994  b82c01             mov    ax, 0x12c               
  03997  50                 push   ax                      
  03998  e8ede7             call   0x2188                  
  0399b  5f                 pop    di                      
  0399c  816efef401         sub    word ptr [bp - 2], 0x1f4
  039a1  ebe3               jmp    0x3986                  
  039a3  e857e7             call   0x20fd                  
  039a6  8be5               mov    sp, bp                  
  039a8  5d                 pop    bp                      
  039a9  c3                 ret                            

ovl1_fn_39aa:
  039aa  55                 push   bp                      
  039ab  8bec               mov    bp, sp                  
  039ad  83ec02             sub    sp, 2                   
  039b0  e8e744             call   0x7e9a                  
  039b3  0bc0               or     ax, ax                  
  039b5  75f9               jne    0x39b0                  
  039b7  33c0               xor    ax, ax                  
  039b9  89063105           mov    word ptr [0x531], ax    
  039bd  89062f05           mov    word ptr [0x52f], ax    
  039c1  b8680b             mov    ax, 0xb68               
  039c4  89066888           mov    word ptr [0x8868], ax   
  039c8  33c0               xor    ax, ax                  
  039ca  89066a88           mov    word ptr [0x886a], ax   
  039ce  813ecea50000       cmp    word ptr [0xa5ce], 0    
  039d4  7544               jne    0x3a1a                  
  039d6  b8069b             mov    ax, 0x9b06              
  039d9  8906cea6           mov    word ptr [0xa6ce], ax   
  039dd  b8680b             mov    ax, 0xb68               
  039e0  50                 push   ax                      
  039e1  b89600             mov    ax, 0x96                
  039e4  50                 push   ax                      
  039e5  e87b47             call   0x8163                  
  039e8  5f                 pop    di                      
  039e9  5f                 pop    di                      
  039ea  09c0               or     ax, ax                  
  039ec  7507               jne    0x39f5                  
  039ee  b80f27             mov    ax, 0x270f              
  039f1  8be5               mov    sp, bp                  
  039f3  5d                 pop    bp                      
  039f4  c3                 ret                            
  039f5  b8680b             mov    ax, 0xb68               
  039f8  05401f             add    ax, 0x1f40              
  039fb  50                 push   ax                      
  039fc  b89900             mov    ax, 0x99                
  039ff  50                 push   ax                      
  03a00  e86047             call   0x8163                  
  03a03  5f                 pop    di                      
  03a04  5f                 pop    di                      
  03a05  09c0               or     ax, ax                  
  03a07  7507               jne    0x3a10                  
  03a09  b80f27             mov    ax, 0x270f              
  03a0c  8be5               mov    sp, bp                  
  03a0e  5d                 pop    bp                      
  03a0f  c3                 ret                            
  03a10  b80100             mov    ax, 1                   
  03a13  8906cea5           mov    word ptr [0xa5ce], ax   
  03a17  e84dfb             call   0x3567                  

;==============================================================================
; OVERLAY 1 entry ovl1_fn_39aa   (file 0x03BAA, loads to seg-offset 0x39AA)
;==============================================================================

ovl1_fn_39aa:
  039aa  55                 push   bp                      
  039ab  8bec               mov    bp, sp                  
  039ad  83ec02             sub    sp, 2                   
  039b0  e8e744             call   0x7e9a                  
  039b3  0bc0               or     ax, ax                  
  039b5  75f9               jne    0x39b0                  
  039b7  33c0               xor    ax, ax                  
  039b9  89063105           mov    word ptr [0x531], ax    
  039bd  89062f05           mov    word ptr [0x52f], ax    
  039c1  b8680b             mov    ax, 0xb68               
  039c4  89066888           mov    word ptr [0x8868], ax   
  039c8  33c0               xor    ax, ax                  
  039ca  89066a88           mov    word ptr [0x886a], ax   
  039ce  813ecea50000       cmp    word ptr [0xa5ce], 0    
  039d4  7544               jne    0x3a1a                  
  039d6  b8069b             mov    ax, 0x9b06              
  039d9  8906cea6           mov    word ptr [0xa6ce], ax   
  039dd  b8680b             mov    ax, 0xb68               
  039e0  50                 push   ax                      
  039e1  b89600             mov    ax, 0x96                
  039e4  50                 push   ax                      
  039e5  e87b47             call   0x8163                  
  039e8  5f                 pop    di                      
  039e9  5f                 pop    di                      
  039ea  09c0               or     ax, ax                  
  039ec  7507               jne    0x39f5                  
  039ee  b80f27             mov    ax, 0x270f              
  039f1  8be5               mov    sp, bp                  
  039f3  5d                 pop    bp                      
  039f4  c3                 ret                            
  039f5  b8680b             mov    ax, 0xb68               
  039f8  05401f             add    ax, 0x1f40              
  039fb  50                 push   ax                      
  039fc  b89900             mov    ax, 0x99                
  039ff  50                 push   ax                      
  03a00  e86047             call   0x8163                  
  03a03  5f                 pop    di                      
  03a04  5f                 pop    di                      
  03a05  09c0               or     ax, ax                  
  03a07  7507               jne    0x3a10                  
  03a09  b80f27             mov    ax, 0x270f              
  03a0c  8be5               mov    sp, bp                  
  03a0e  5d                 pop    bp                      
  03a0f  c3                 ret                            
  03a10  b80100             mov    ax, 1                   
  03a13  8906cea5           mov    word ptr [0xa5ce], ax   
  03a17  e84dfb             call   0x3567                  
  03a1a  b80200             mov    ax, 2                   
  03a1d  8906c0a5           mov    word ptr [0xa5c0], ax   
  03a21  33c0               xor    ax, ax                  
  03a23  8906c5a8           mov    word ptr [0xa8c5], ax   
  03a27  8906c3a8           mov    word ptr [0xa8c3], ax   
  03a2b  8b066888           mov    ax, word ptr [0x8868]   
  03a2f  052608             add    ax, 0x826               
  03a32  8906c7a8           mov    word ptr [0xa8c7], ax   
  03a36  8906c1a8           mov    word ptr [0xa8c1], ax   
  03a3a  813e92a60000       cmp    word ptr [0xa692], 0    
  03a40  740b               je     0x3a4d                  
  03a42  813e92a60a00       cmp    word ptr [0xa692], 0xa  
  03a48  7403               je     0x3a4d                  
  03a4a  e98400             jmp    0x3ad1                  
  03a4d  813e92a60a00       cmp    word ptr [0xa692], 0xa  
  03a53  7503               jne    0x3a58                  
  03a55  e84805             call   0x3fa0                  
  03a58  e83e3a             call   0x7499                  
  03a5b  e8733a             call   0x74d1                  
  03a5e  33c0               xor    ax, ax                  
  03a60  50                 push   ax                      
  03a61  e8f7ca             call   0x55b                   
  03a64  5f                 pop    di                      
  03a65  e81030             call   0x6a78                  
  03a68  813e92a60a00       cmp    word ptr [0xa692], 0xa  
  03a6e  755e               jne    0x3ace                  
  03a70  b81700             mov    ax, 0x17                
  03a73  50                 push   ax                      
  03a74  e867e4             call   0x1ede                  
  03a77  5f                 pop    di                      
  03a78  b81800             mov    ax, 0x18                
  03a7b  50                 push   ax                      
  03a7c  e85fe4             call   0x1ede                  
  03a7f  5f                 pop    di                      
  03a80  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  03a84  05ae0d             add    ax, 0xdae               
  03a87  50                 push   ax                      
  03a88  e8daea             call   0x2565                  
  03a8b  5f                 pop    di                      
  03a8c  813e2f050000       cmp    word ptr [0x52f], 0     
  03a92  74f8               je     0x3a8c                  
  03a94  813e2f051500       cmp    word ptr [0x52f], 0x15  
  03a9a  7516               jne    0x3ab2                  
  03a9c  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  03aa0  055a0e             add    ax, 0xe5a               
  03aa3  50                 push   ax                      
  03aa4  e8beea             call   0x2565                  
  03aa7  5f                 pop    di                      
  03aa8  b86400             mov    ax, 0x64                
  03aab  50                 push   ax                      
  03aac  e81405             call   0x3fc3   ; -> ovl1_fn_3fc3
  03aaf  5f                 pop    di                      
  03ab0  eb1c               jmp    0x3ace                  
  03ab2  813e2f053100       cmp    word ptr [0x52f], 0x31  
  03ab8  7512               jne    0x3acc                  
  03aba  b89c00             mov    ax, 0x9c                
  03abd  50                 push   ax                      
  03abe  e8a4ea             call   0x2565                  
  03ac1  5f                 pop    di                      
  03ac2  b86500             mov    ax, 0x65                
  03ac5  50                 push   ax                      
  03ac6  e8fa04             call   0x3fc3   ; -> ovl1_fn_3fc3
  03ac9  5f                 pop    di                      
  03aca  eb02               jmp    0x3ace                  
  03acc  ebaa               jmp    0x3a78                  
  03ace  e90502             jmp    0x3cd6                  
  03ad1  813e92a60100       cmp    word ptr [0xa692], 1    
  03ad7  7408               je     0x3ae1                  
  03ad9  813e92a60500       cmp    word ptr [0xa692], 5    
  03adf  7502               jne    0x3ae3                  
  03ae1  eb0b               jmp    0x3aee                  
  03ae3  813e92a60700       cmp    word ptr [0xa692], 7    
  03ae9  7403               je     0x3aee                  
  03aeb  e9c800             jmp    0x3bb6                  
  03aee  e84c40             call   0x7b3d                  
  03af1  8946fe             mov    word ptr [bp - 2], ax   
  03af4  09c0               or     ax, ax                  
  03af6  7509               jne    0x3b01                  
  03af8  b80f27             mov    ax, 0x270f              
  03afb  8946fe             mov    word ptr [bp - 2], ax   
  03afe  e91002             jmp    0x3d11                  
  03b01  817efe0200         cmp    word ptr [bp - 2], 2    
  03b06  7515               jne    0x3b1d                  
  03b08  e8dafc             call   0x37e5                  
  03b0b  0bc0               or     ax, ax                  
  03b0d  7407               je     0x3b16                  
  03b0f  b80e27             mov    ax, 0x270e              
  03b12  8be5               mov    sp, bp                  
  03b14  5d                 pop    bp                      
  03b15  c3                 ret                            
  03b16  b86400             mov    ax, 0x64                
  03b19  8be5               mov    sp, bp                  
  03b1b  5d                 pop    bp                      
  03b1c  c3                 ret                            
  03b1d  817efe0500         cmp    word ptr [bp - 2], 5    
  03b22  7507               jne    0x3b2b                  
  03b24  b80500             mov    ax, 5                   
  03b27  8be5               mov    sp, bp                  
  03b29  5d                 pop    bp                      

;==============================================================================
; OVERLAY 1 entry ovl1_fn_3d1e   (file 0x03F1E, loads to seg-offset 0x3D1E)
;==============================================================================

ovl1_fn_3d1e:
  03d1e  55                 push   bp                      
  03d1f  8bec               mov    bp, sp                  
  03d21  83ec02             sub    sp, 2                   
  03d24  33c0               xor    ax, ax                  
  03d26  8906cea5           mov    word ptr [0xa5ce], ax   
  03d2a  b8680b             mov    ax, 0xb68               
  03d2d  89066888           mov    word ptr [0x8868], ax   
  03d31  b8680b             mov    ax, 0xb68               
  03d34  05401f             add    ax, 0x1f40              
  03d37  8906cea6           mov    word ptr [0xa6ce], ax   
  03d3b  b8680b             mov    ax, 0xb68               
  03d3e  50                 push   ax                      
  03d3f  b89f00             mov    ax, 0x9f                
  03d42  50                 push   ax                      
  03d43  e81d44             call   0x8163                  
  03d46  5f                 pop    di                      
  03d47  5f                 pop    di                      
  03d48  09c0               or     ax, ax                  
  03d4a  7507               jne    0x3d53                  
  03d4c  b80f27             mov    ax, 0x270f              
  03d4f  8be5               mov    sp, bp                  
  03d51  5d                 pop    bp                      
  03d52  c3                 ret                            
  03d53  b8c800             mov    ax, 0xc8                
  03d56  50                 push   ax                      
  03d57  e84fda             call   0x17a9                  
  03d5a  5f                 pop    di                      
  03d5b  b80f00             mov    ax, 0xf                 
  03d5e  50                 push   ax                      
  03d5f  b00c               mov    al, 0xc                 
  03d61  50                 push   ax                      
  03d62  b002               mov    al, 2                   
  03d64  50                 push   ax                      
  03d65  33c0               xor    ax, ax                  
  03d67  50                 push   ax                      
  03d68  b00f               mov    al, 0xf                 
  03d6a  50                 push   ax                      
  03d6b  b007               mov    al, 7                   
  03d6d  50                 push   ax                      
  03d6e  b00f               mov    al, 0xf                 
  03d70  50                 push   ax                      
  03d71  33c0               xor    ax, ax                  
  03d73  50                 push   ax                      
  03d74  b00f               mov    al, 0xf                 
  03d76  50                 push   ax                      
  03d77  b00d               mov    al, 0xd                 
  03d79  50                 push   ax                      
  03d7a  b00b               mov    al, 0xb                 
  03d7c  50                 push   ax                      
  03d7d  33c0               xor    ax, ax                  
  03d7f  50                 push   ax                      
  03d80  e883c8             call   0x606                   
  03d83  83c418             add    sp, 0x18                
  03d86  b8401f             mov    ax, 0x1f40              
  03d89  50                 push   ax                      
  03d8a  ff36cea6           push   word ptr [0xa6ce]       
  03d8e  b8a200             mov    ax, 0xa2                
  03d91  50                 push   ax                      
  03d92  e858da             call   0x17ed                  
  03d95  83c406             add    sp, 6                   
  03d98  09c0               or     ax, ax                  
  03d9a  7507               jne    0x3da3                  
  03d9c  b80f27             mov    ax, 0x270f              
  03d9f  8be5               mov    sp, bp                  
  03da1  5d                 pop    bp                      
  03da2  c3                 ret                            
  03da3  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03da7  05e301             add    ax, 0x1e3               
  03daa  89068fa9           mov    word ptr [0xa98f], ax   
  03dae  33c0               xor    ax, ax                  
  03db0  8946fe             mov    word ptr [bp - 2], ax   
  03db3  817efe0600         cmp    word ptr [bp - 2], 6    
  03db8  7d22               jge    0x3ddc                  
  03dba  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03dbe  8b368fa9           mov    si, word ptr [0xa98f]   
  03dc2  8b56fe             mov    dx, word ptr [bp - 2]   
  03dc5  d1e2               shl    dx, 1                   
  03dc7  03f2               add    si, dx                  
  03dc9  0304               add    ax, word ptr [si]       
  03dcb  be82a6             mov    si, 0xa682              
  03dce  8b56fe             mov    dx, word ptr [bp - 2]   
  03dd1  d1e2               shl    dx, 1                   
  03dd3  03f2               add    si, dx                  
  03dd5  8904               mov    word ptr [si], ax       
  03dd7  ff46fe             inc    word ptr [bp - 2]       
  03dda  ebd7               jmp    0x3db3                  
  03ddc  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03de0  059b01             add    ax, 0x19b               
  03de3  890629a9           mov    word ptr [0xa929], ax   
  03de7  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03deb  05a701             add    ax, 0x1a7               
  03dee  89062ba9           mov    word ptr [0xa92b], ax   
  03df2  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03df6  05b301             add    ax, 0x1b3               
  03df9  89062da9           mov    word ptr [0xa92d], ax   
  03dfd  ff367ea6           push   word ptr [0xa67e]       
  03e01  e81422             call   0x6018                  
  03e04  8be5               mov    sp, bp                  
  03e06  5d                 pop    bp                      
  03e07  c3                 ret                            

ovl1_fn_3e08:
  03e08  55                 push   bp                      
  03e09  8bec               mov    bp, sp                  
  03e0b  83ec02             sub    sp, 2                   
  03e0e  b80900             mov    ax, 9                   
  03e11  890692a6           mov    word ptr [0xa692], ax   
  03e15  e8f009             call   0x4808   ; -> ovl1_fn_4808
  03e18  0bc0               or     ax, ax                  
  03e1a  7407               je     0x3e23                  
  03e1c  b80f27             mov    ax, 0x270f              
  03e1f  8be5               mov    sp, bp                  
  03e21  5d                 pop    bp                      
  03e22  c3                 ret                            
  03e23  b84803             mov    ax, 0x348               
  03e26  50                 push   ax                      
  03e27  b88297             mov    ax, 0x9782              
  03e2a  50                 push   ax                      
  03e2b  b82000             mov    ax, 0x20                
  03e2e  50                 push   ax                      
  03e2f  e8e3e0             call   0x1f15                  
  03e32  83c406             add    sp, 6                   
  03e35  b80b00             mov    ax, 0xb                 
  03e38  890692a6           mov    word ptr [0xa692], ax   
  03e3c  e86bfb             call   0x39aa   ; -> ovl1_fn_39aa
  03e3f  8946fe             mov    word ptr [bp - 2], ax   
  03e42  0bc0               or     ax, ax                  
  03e44  7407               je     0x3e4d                  
  03e46  8b46fe             mov    ax, word ptr [bp - 2]   
  03e49  8be5               mov    sp, bp                  
  03e4b  5d                 pop    bp                      
  03e4c  c3                 ret                            
  03e4d  33c0               xor    ax, ax                  
  03e4f  890692a6           mov    word ptr [0xa692], ax   
  03e53  e854fb             call   0x39aa   ; -> ovl1_fn_39aa
  03e56  8946fe             mov    word ptr [bp - 2], ax   
  03e59  0bc0               or     ax, ax                  
  03e5b  7407               je     0x3e64                  
  03e5d  8b46fe             mov    ax, word ptr [bp - 2]   
  03e60  8be5               mov    sp, bp                  
  03e62  5d                 pop    bp                      
  03e63  c3                 ret                            
  03e64  e87afa             call   0x38e1                  
  03e67  813edca60200       cmp    word ptr [0xa6dc], 2    
  03e6d  7508               jne    0x3e77                  
  03e6f  33c0               xor    ax, ax                  
  03e71  8906dca6           mov    word ptr [0xa6dc], ax   
  03e75  ebed               jmp    0x3e64                  
  03e77  ff3694a6           push   word ptr [0xa694]       
  03e7b  e8f33b             call   0x7a71                  
  03e7e  5f                 pop    di                      
  03e7f  09c0               or     ax, ax                  
  03e81  7507               jne    0x3e8a                  
  03e83  b80e27             mov    ax, 0x270e              
  03e86  8be5               mov    sp, bp                  
  03e88  5d                 pop    bp                      
  03e89  c3                 ret                            
  03e8a  b80300             mov    ax, 3                   
  03e8d  890692a6           mov    word ptr [0xa692], ax   
  03e91  e816fb             call   0x39aa   ; -> ovl1_fn_39aa
  03e94  0bc0               or     ax, ax                  
  03e96  7518               jne    0x3eb0                  
  03e98  b80c00             mov    ax, 0xc                 
  03e9b  50                 push   ax                      
  03e9c  b006               mov    al, 6                   

;==============================================================================
; OVERLAY 1 entry ovl1_fn_3e08   (file 0x04008, loads to seg-offset 0x3E08)
;==============================================================================

ovl1_fn_3e08:
  03e08  55                 push   bp                      
  03e09  8bec               mov    bp, sp                  
  03e0b  83ec02             sub    sp, 2                   
  03e0e  b80900             mov    ax, 9                   
  03e11  890692a6           mov    word ptr [0xa692], ax   
  03e15  e8f009             call   0x4808   ; -> ovl1_fn_4808
  03e18  0bc0               or     ax, ax                  
  03e1a  7407               je     0x3e23                  
  03e1c  b80f27             mov    ax, 0x270f              
  03e1f  8be5               mov    sp, bp                  
  03e21  5d                 pop    bp                      
  03e22  c3                 ret                            
  03e23  b84803             mov    ax, 0x348               
  03e26  50                 push   ax                      
  03e27  b88297             mov    ax, 0x9782              
  03e2a  50                 push   ax                      
  03e2b  b82000             mov    ax, 0x20                
  03e2e  50                 push   ax                      
  03e2f  e8e3e0             call   0x1f15                  
  03e32  83c406             add    sp, 6                   
  03e35  b80b00             mov    ax, 0xb                 
  03e38  890692a6           mov    word ptr [0xa692], ax   
  03e3c  e86bfb             call   0x39aa   ; -> ovl1_fn_39aa
  03e3f  8946fe             mov    word ptr [bp - 2], ax   
  03e42  0bc0               or     ax, ax                  
  03e44  7407               je     0x3e4d                  
  03e46  8b46fe             mov    ax, word ptr [bp - 2]   
  03e49  8be5               mov    sp, bp                  
  03e4b  5d                 pop    bp                      
  03e4c  c3                 ret                            
  03e4d  33c0               xor    ax, ax                  
  03e4f  890692a6           mov    word ptr [0xa692], ax   
  03e53  e854fb             call   0x39aa   ; -> ovl1_fn_39aa
  03e56  8946fe             mov    word ptr [bp - 2], ax   
  03e59  0bc0               or     ax, ax                  
  03e5b  7407               je     0x3e64                  
  03e5d  8b46fe             mov    ax, word ptr [bp - 2]   
  03e60  8be5               mov    sp, bp                  
  03e62  5d                 pop    bp                      
  03e63  c3                 ret                            
  03e64  e87afa             call   0x38e1                  
  03e67  813edca60200       cmp    word ptr [0xa6dc], 2    
  03e6d  7508               jne    0x3e77                  
  03e6f  33c0               xor    ax, ax                  
  03e71  8906dca6           mov    word ptr [0xa6dc], ax   
  03e75  ebed               jmp    0x3e64                  
  03e77  ff3694a6           push   word ptr [0xa694]       
  03e7b  e8f33b             call   0x7a71                  
  03e7e  5f                 pop    di                      
  03e7f  09c0               or     ax, ax                  
  03e81  7507               jne    0x3e8a                  
  03e83  b80e27             mov    ax, 0x270e              
  03e86  8be5               mov    sp, bp                  
  03e88  5d                 pop    bp                      
  03e89  c3                 ret                            
  03e8a  b80300             mov    ax, 3                   
  03e8d  890692a6           mov    word ptr [0xa692], ax   
  03e91  e816fb             call   0x39aa   ; -> ovl1_fn_39aa
  03e94  0bc0               or     ax, ax                  
  03e96  7518               jne    0x3eb0                  
  03e98  b80c00             mov    ax, 0xc                 
  03e9b  50                 push   ax                      
  03e9c  b006               mov    al, 6                   
  03e9e  50                 push   ax                      
  03e9f  e8bae1             call   0x205c                  
  03ea2  5f                 pop    di                      
  03ea3  5f                 pop    di                      
  03ea4  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  03ea8  059a0c             add    ax, 0xc9a               
  03eab  50                 push   ax                      
  03eac  e8b6e6             call   0x2565                  
  03eaf  5f                 pop    di                      
  03eb0  33c0               xor    ax, ax                  
  03eb2  8be5               mov    sp, bp                  
  03eb4  5d                 pop    bp                      
  03eb5  c3                 ret                            

ovl1_fn_3eb6:
  03eb6  55                 push   bp                      
  03eb7  8bec               mov    bp, sp                  
  03eb9  83ec02             sub    sp, 2                   
  03ebc  33c0               xor    ax, ax                  
  03ebe  8906cea5           mov    word ptr [0xa5ce], ax   
  03ec2  b8680b             mov    ax, 0xb68               
  03ec5  89066888           mov    word ptr [0x8868], ax   
  03ec9  b8680b             mov    ax, 0xb68               
  03ecc  05401f             add    ax, 0x1f40              
  03ecf  8906cea6           mov    word ptr [0xa6ce], ax   
  03ed3  b8680b             mov    ax, 0xb68               
  03ed6  50                 push   ax                      
  03ed7  b8a500             mov    ax, 0xa5                
  03eda  50                 push   ax                      
  03edb  e88542             call   0x8163                  
  03ede  5f                 pop    di                      
  03edf  5f                 pop    di                      
  03ee0  09c0               or     ax, ax                  
  03ee2  7507               jne    0x3eeb                  
  03ee4  b80f27             mov    ax, 0x270f              
  03ee7  8be5               mov    sp, bp                  
  03ee9  5d                 pop    bp                      
  03eea  c3                 ret                            
  03eeb  b8c800             mov    ax, 0xc8                
  03eee  50                 push   ax                      
  03eef  e8b7d8             call   0x17a9                  
  03ef2  5f                 pop    di                      
  03ef3  b80f00             mov    ax, 0xf                 
  03ef6  50                 push   ax                      
  03ef7  b00c               mov    al, 0xc                 
  03ef9  50                 push   ax                      
  03efa  b002               mov    al, 2                   
  03efc  50                 push   ax                      
  03efd  33c0               xor    ax, ax                  
  03eff  50                 push   ax                      
  03f00  b00f               mov    al, 0xf                 
  03f02  50                 push   ax                      
  03f03  b00b               mov    al, 0xb                 
  03f05  50                 push   ax                      
  03f06  b007               mov    al, 7                   
  03f08  50                 push   ax                      
  03f09  33c0               xor    ax, ax                  
  03f0b  50                 push   ax                      
  03f0c  b00f               mov    al, 0xf                 
  03f0e  50                 push   ax                      
  03f0f  b00d               mov    al, 0xd                 
  03f11  50                 push   ax                      
  03f12  b00b               mov    al, 0xb                 
  03f14  50                 push   ax                      
  03f15  33c0               xor    ax, ax                  
  03f17  50                 push   ax                      
  03f18  e8ebc6             call   0x606                   
  03f1b  83c418             add    sp, 0x18                
  03f1e  b8401f             mov    ax, 0x1f40              
  03f21  50                 push   ax                      
  03f22  ff36cea6           push   word ptr [0xa6ce]       
  03f26  b8a800             mov    ax, 0xa8                
  03f29  50                 push   ax                      
  03f2a  e8c0d8             call   0x17ed                  
  03f2d  83c406             add    sp, 6                   
  03f30  09c0               or     ax, ax                  
  03f32  7507               jne    0x3f3b                  
  03f34  b80f27             mov    ax, 0x270f              
  03f37  8be5               mov    sp, bp                  
  03f39  5d                 pop    bp                      
  03f3a  c3                 ret                            
  03f3b  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03f3f  05ef01             add    ax, 0x1ef               
  03f42  89068fa9           mov    word ptr [0xa98f], ax   
  03f46  33c0               xor    ax, ax                  
  03f48  8946fe             mov    word ptr [bp - 2], ax   
  03f4b  817efe0600         cmp    word ptr [bp - 2], 6    
  03f50  7d22               jge    0x3f74                  
  03f52  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03f56  8b368fa9           mov    si, word ptr [0xa98f]   
  03f5a  8b56fe             mov    dx, word ptr [bp - 2]   
  03f5d  d1e2               shl    dx, 1                   
  03f5f  03f2               add    si, dx                  
  03f61  0304               add    ax, word ptr [si]       
  03f63  be82a6             mov    si, 0xa682              
  03f66  8b56fe             mov    dx, word ptr [bp - 2]   
  03f69  d1e2               shl    dx, 1                   
  03f6b  03f2               add    si, dx                  
  03f6d  8904               mov    word ptr [si], ax       
  03f6f  ff46fe             inc    word ptr [bp - 2]       
  03f72  ebd7               jmp    0x3f4b                  
  03f74  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03f78  05bf01             add    ax, 0x1bf               
  03f7b  890629a9           mov    word ptr [0xa929], ax   
  03f7f  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03f83  05cb01             add    ax, 0x1cb               

;==============================================================================
; OVERLAY 1 entry ovl1_fn_3eb6   (file 0x040B6, loads to seg-offset 0x3EB6)
;==============================================================================

ovl1_fn_3eb6:
  03eb6  55                 push   bp                      
  03eb7  8bec               mov    bp, sp                  
  03eb9  83ec02             sub    sp, 2                   
  03ebc  33c0               xor    ax, ax                  
  03ebe  8906cea5           mov    word ptr [0xa5ce], ax   
  03ec2  b8680b             mov    ax, 0xb68               
  03ec5  89066888           mov    word ptr [0x8868], ax   
  03ec9  b8680b             mov    ax, 0xb68               
  03ecc  05401f             add    ax, 0x1f40              
  03ecf  8906cea6           mov    word ptr [0xa6ce], ax   
  03ed3  b8680b             mov    ax, 0xb68               
  03ed6  50                 push   ax                      
  03ed7  b8a500             mov    ax, 0xa5                
  03eda  50                 push   ax                      
  03edb  e88542             call   0x8163                  
  03ede  5f                 pop    di                      
  03edf  5f                 pop    di                      
  03ee0  09c0               or     ax, ax                  
  03ee2  7507               jne    0x3eeb                  
  03ee4  b80f27             mov    ax, 0x270f              
  03ee7  8be5               mov    sp, bp                  
  03ee9  5d                 pop    bp                      
  03eea  c3                 ret                            
  03eeb  b8c800             mov    ax, 0xc8                
  03eee  50                 push   ax                      
  03eef  e8b7d8             call   0x17a9                  
  03ef2  5f                 pop    di                      
  03ef3  b80f00             mov    ax, 0xf                 
  03ef6  50                 push   ax                      
  03ef7  b00c               mov    al, 0xc                 
  03ef9  50                 push   ax                      
  03efa  b002               mov    al, 2                   
  03efc  50                 push   ax                      
  03efd  33c0               xor    ax, ax                  
  03eff  50                 push   ax                      
  03f00  b00f               mov    al, 0xf                 
  03f02  50                 push   ax                      
  03f03  b00b               mov    al, 0xb                 
  03f05  50                 push   ax                      
  03f06  b007               mov    al, 7                   
  03f08  50                 push   ax                      
  03f09  33c0               xor    ax, ax                  
  03f0b  50                 push   ax                      
  03f0c  b00f               mov    al, 0xf                 
  03f0e  50                 push   ax                      
  03f0f  b00d               mov    al, 0xd                 
  03f11  50                 push   ax                      
  03f12  b00b               mov    al, 0xb                 
  03f14  50                 push   ax                      
  03f15  33c0               xor    ax, ax                  
  03f17  50                 push   ax                      
  03f18  e8ebc6             call   0x606                   
  03f1b  83c418             add    sp, 0x18                
  03f1e  b8401f             mov    ax, 0x1f40              
  03f21  50                 push   ax                      
  03f22  ff36cea6           push   word ptr [0xa6ce]       
  03f26  b8a800             mov    ax, 0xa8                
  03f29  50                 push   ax                      
  03f2a  e8c0d8             call   0x17ed                  
  03f2d  83c406             add    sp, 6                   
  03f30  09c0               or     ax, ax                  
  03f32  7507               jne    0x3f3b                  
  03f34  b80f27             mov    ax, 0x270f              
  03f37  8be5               mov    sp, bp                  
  03f39  5d                 pop    bp                      
  03f3a  c3                 ret                            
  03f3b  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03f3f  05ef01             add    ax, 0x1ef               
  03f42  89068fa9           mov    word ptr [0xa98f], ax   
  03f46  33c0               xor    ax, ax                  
  03f48  8946fe             mov    word ptr [bp - 2], ax   
  03f4b  817efe0600         cmp    word ptr [bp - 2], 6    
  03f50  7d22               jge    0x3f74                  
  03f52  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03f56  8b368fa9           mov    si, word ptr [0xa98f]   
  03f5a  8b56fe             mov    dx, word ptr [bp - 2]   
  03f5d  d1e2               shl    dx, 1                   
  03f5f  03f2               add    si, dx                  
  03f61  0304               add    ax, word ptr [si]       
  03f63  be82a6             mov    si, 0xa682              
  03f66  8b56fe             mov    dx, word ptr [bp - 2]   
  03f69  d1e2               shl    dx, 1                   
  03f6b  03f2               add    si, dx                  
  03f6d  8904               mov    word ptr [si], ax       
  03f6f  ff46fe             inc    word ptr [bp - 2]       
  03f72  ebd7               jmp    0x3f4b                  
  03f74  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03f78  05bf01             add    ax, 0x1bf               
  03f7b  890629a9           mov    word ptr [0xa929], ax   
  03f7f  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03f83  05cb01             add    ax, 0x1cb               
  03f86  89062ba9           mov    word ptr [0xa92b], ax   
  03f8a  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03f8e  05d701             add    ax, 0x1d7               
  03f91  89062da9           mov    word ptr [0xa92d], ax   
  03f95  ff3680a6           push   word ptr [0xa680]       
  03f99  e87c20             call   0x6018                  
  03f9c  8be5               mov    sp, bp                  
  03f9e  5d                 pop    bp                      
  03f9f  c3                 ret                            
  03fa0  55                 push   bp                      
  03fa1  8bec               mov    bp, sp                  
  03fa3  83ec02             sub    sp, 2                   
  03fa6  b81800             mov    ax, 0x18                
  03fa9  8946fe             mov    word ptr [bp - 2], ax   
  03fac  817efe0000         cmp    word ptr [bp - 2], 0    
  03fb1  7c0c               jl     0x3fbf                  
  03fb3  ff76fe             push   word ptr [bp - 2]       
  03fb6  e825df             call   0x1ede                  
  03fb9  5f                 pop    di                      
  03fba  ff4efe             dec    word ptr [bp - 2]       
  03fbd  ebed               jmp    0x3fac                  
  03fbf  8be5               mov    sp, bp                  
  03fc1  5d                 pop    bp                      
  03fc2  c3                 ret                            

ovl1_fn_3fc3:
  03fc3  55                 push   bp                      
  03fc4  8bec               mov    bp, sp                  
  03fc6  e818f9             call   0x38e1                  
  03fc9  813edca60200       cmp    word ptr [0xa6dc], 2    
  03fcf  7508               jne    0x3fd9                  
  03fd1  33c0               xor    ax, ax                  
  03fd3  8906dca6           mov    word ptr [0xa6dc], ax   
  03fd7  ebed               jmp    0x3fc6                  
  03fd9  817e046400         cmp    word ptr [bp + 4], 0x64 
  03fde  750a               jne    0x3fea                  
  03fe0  ff3694a6           push   word ptr [0xa694]       
  03fe4  e88a3a             call   0x7a71                  
  03fe7  5f                 pop    di                      
  03fe8  eb0f               jmp    0x3ff9                  
  03fea  817e046500         cmp    word ptr [bp + 4], 0x65 
  03fef  7508               jne    0x3ff9                  
  03ff1  ff3694a6           push   word ptr [0xa694]       
  03ff5  e87838             call   0x7870                  
  03ff8  5f                 pop    di                      
  03ff9  e89ff8             call   0x389b   ; -> ovl1_fn_389b
  03ffc  8be5               mov    sp, bp                  
  03ffe  5d                 pop    bp                      
  03fff  c3                 ret                            

ovl1_fn_4000:
  04000  55                 push   bp                      
  04001  8bec               mov    bp, sp                  
  04003  83ec06             sub    sp, 6                   
  04006  8b06dca6           mov    ax, word ptr [0xa6dc]   
  0400a  8946fc             mov    word ptr [bp - 4], ax   
  0400d  33c0               xor    ax, ax                  
  0400f  8906dca6           mov    word ptr [0xa6dc], ax   
  04013  b005               mov    al, 5                   
  04015  890692a6           mov    word ptr [0xa692], ax   
  04019  e88ef9             call   0x39aa   ; -> ovl1_fn_39aa
  0401c  8946fa             mov    word ptr [bp - 6], ax   
  0401f  0bc0               or     ax, ax                  
  04021  740a               je     0x402d                  
  04023  e875f8             call   0x389b   ; -> ovl1_fn_389b
  04026  8b46fa             mov    ax, word ptr [bp - 6]   
  04029  8be5               mov    sp, bp                  
  0402b  5d                 pop    bp                      
  0402c  c3                 ret                            
  0402d  ff36f5a6           push   word ptr [0xa6f5]       
  04031  e814c1             call   0x148                   
  04034  5f                 pop    di                      

;==============================================================================
; OVERLAY 1 entry ovl1_fn_3fc3   (file 0x041C3, loads to seg-offset 0x3FC3)
;==============================================================================

ovl1_fn_3fc3:
  03fc3  55                 push   bp                      
  03fc4  8bec               mov    bp, sp                  
  03fc6  e818f9             call   0x38e1                  
  03fc9  813edca60200       cmp    word ptr [0xa6dc], 2    
  03fcf  7508               jne    0x3fd9                  
  03fd1  33c0               xor    ax, ax                  
  03fd3  8906dca6           mov    word ptr [0xa6dc], ax   
  03fd7  ebed               jmp    0x3fc6                  
  03fd9  817e046400         cmp    word ptr [bp + 4], 0x64 
  03fde  750a               jne    0x3fea                  
  03fe0  ff3694a6           push   word ptr [0xa694]       
  03fe4  e88a3a             call   0x7a71                  
  03fe7  5f                 pop    di                      
  03fe8  eb0f               jmp    0x3ff9                  
  03fea  817e046500         cmp    word ptr [bp + 4], 0x65 
  03fef  7508               jne    0x3ff9                  
  03ff1  ff3694a6           push   word ptr [0xa694]       
  03ff5  e87838             call   0x7870                  
  03ff8  5f                 pop    di                      
  03ff9  e89ff8             call   0x389b   ; -> ovl1_fn_389b
  03ffc  8be5               mov    sp, bp                  
  03ffe  5d                 pop    bp                      
  03fff  c3                 ret                            

ovl1_fn_4000:
  04000  55                 push   bp                      
  04001  8bec               mov    bp, sp                  
  04003  83ec06             sub    sp, 6                   
  04006  8b06dca6           mov    ax, word ptr [0xa6dc]   
  0400a  8946fc             mov    word ptr [bp - 4], ax   
  0400d  33c0               xor    ax, ax                  
  0400f  8906dca6           mov    word ptr [0xa6dc], ax   
  04013  b005               mov    al, 5                   
  04015  890692a6           mov    word ptr [0xa692], ax   
  04019  e88ef9             call   0x39aa   ; -> ovl1_fn_39aa
  0401c  8946fa             mov    word ptr [bp - 6], ax   
  0401f  0bc0               or     ax, ax                  
  04021  740a               je     0x402d                  
  04023  e875f8             call   0x389b   ; -> ovl1_fn_389b
  04026  8b46fa             mov    ax, word ptr [bp - 6]   
  04029  8be5               mov    sp, bp                  
  0402b  5d                 pop    bp                      
  0402c  c3                 ret                            
  0402d  ff36f5a6           push   word ptr [0xa6f5]       
  04031  e814c1             call   0x148                   
  04034  5f                 pop    di                      
  04035  83f801             cmp    ax, 1                   
  04038  7506               jne    0x4040                  
  0403a  33c0               xor    ax, ax                  
  0403c  8be5               mov    sp, bp                  
  0403e  5d                 pop    bp                      
  0403f  c3                 ret                            
  04040  813ecda80100       cmp    word ptr [0xa8cd], 1    
  04046  7c08               jl     0x4050                  
  04048  813e94a60100       cmp    word ptr [0xa694], 1    
  0404e  7d03               jge    0x4053                  
  04050  e9c200             jmp    0x4115                  
  04053  813e94a60b00       cmp    word ptr [0xa694], 0xb  
  04059  7d53               jge    0x40ae                  
  0405b  b8c108             mov    ax, 0x8c1               
  0405e  50                 push   ax                      
  0405f  bb0300             mov    bx, 3                   
  04062  8b0694a6           mov    ax, word ptr [0xa694]   
  04066  f7eb               imul   bx                      
  04068  5e                 pop    si                      
  04069  03f0               add    si, ax                  
  0406b  8976fe             mov    word ptr [bp - 2], si   
  0406e  b80300             mov    ax, 3                   
  04071  50                 push   ax                      
  04072  b83e0b             mov    ax, 0xb3e               
  04075  50                 push   ax                      
  04076  ff76fe             push   word ptr [bp - 2]       
  04079  e8c7de             call   0x1f43                  
  0407c  83c406             add    sp, 6                   
  0407f  bb0300             mov    bx, 3                   
  04082  b80b00             mov    ax, 0xb                 
  04085  2b0694a6           sub    ax, word ptr [0xa694]   
  04089  f7eb               imul   bx                      
  0408b  50                 push   ax                      
  0408c  ff76fe             push   word ptr [bp - 2]       
  0408f  8b46fe             mov    ax, word ptr [bp - 2]   
  04092  83c003             add    ax, 3                   
  04095  50                 push   ax                      
  04096  e8aade             call   0x1f43                  
  04099  83c406             add    sp, 6                   
  0409c  b80300             mov    ax, 3                   
  0409f  50                 push   ax                      
  040a0  b8e208             mov    ax, 0x8e2               
  040a3  50                 push   ax                      
  040a4  b83e0b             mov    ax, 0xb3e               
  040a7  50                 push   ax                      
  040a8  e898de             call   0x1f43                  
  040ab  83c406             add    sp, 6                   
  040ae  b8819a             mov    ax, 0x9a81              
  040b1  2b067aa6           sub    ax, word ptr [0xa67a]   
  040b5  33d2               xor    dx, dx                  
  040b7  8946fe             mov    word ptr [bp - 2], ax   
  040ba  817efee703         cmp    word ptr [bp - 2], 0x3e7
  040bf  7315               jae    0x40d6                  
  040c1  ff76fe             push   word ptr [bp - 2]       
  040c4  ff367aa6           push   word ptr [0xa67a]       
  040c8  8b067aa6           mov    ax, word ptr [0xa67a]   
  040cc  83c040             add    ax, 0x40                
  040cf  50                 push   ax                      
  040d0  e870de             call   0x1f43                  
  040d3  83c406             add    sp, 6                   
  040d6  b88297             mov    ax, 0x9782              
  040d9  50                 push   ax                      
  040da  bb4000             mov    bx, 0x40                
  040dd  8b06cda8           mov    ax, word ptr [0xa8cd]   
  040e1  f7eb               imul   bx                      
  040e3  83c05f             add    ax, 0x5f                
  040e6  5e                 pop    si                      
  040e7  03f0               add    si, ax                  
  040e9  8976fe             mov    word ptr [bp - 2], si   
  040ec  b82000             mov    ax, 0x20                
  040ef  50                 push   ax                      
  040f0  ff76fe             push   word ptr [bp - 2]       
  040f3  50                 push   ax                      
  040f4  e81ede             call   0x1f15                  
  040f7  83c406             add    sp, 6                   
  040fa  e82800             call   0x4125                  
  040fd  e85935             call   0x7659                  
  04100  e87d2d             call   0x6e80                  
  04103  ff0ecda8           dec    word ptr [0xa8cd]       
  04107  e8ff3c             call   0x7e09                  
  0410a  09c0               or     ax, ax                  
  0410c  7507               jne    0x4115                  
  0410e  b80e27             mov    ax, 0x270e              
  04111  8be5               mov    sp, bp                  
  04113  5d                 pop    bp                      
  04114  c3                 ret                            
  04115  e883f7             call   0x389b   ; -> ovl1_fn_389b
  04118  8b46fc             mov    ax, word ptr [bp - 4]   
  0411b  8906dca6           mov    word ptr [0xa6dc], ax   
  0411f  33c0               xor    ax, ax                  
  04121  8be5               mov    sp, bp                  
  04123  5d                 pop    bp                      
  04124  c3                 ret                            
  04125  55                 push   bp                      
  04126  8bec               mov    bp, sp                  
  04128  83ec02             sub    sp, 2                   
  0412b  33c0               xor    ax, ax                  
  0412d  8946fe             mov    word ptr [bp - 2], ax   
  04130  817efe2100         cmp    word ptr [bp - 2], 0x21 
  04135  7d26               jge    0x415d                  
  04137  b82000             mov    ax, 0x20                
  0413a  50                 push   ax                      
  0413b  e8892f             call   0x70c7                  
  0413e  5f                 pop    di                      

;==============================================================================
; OVERLAY 1 entry ovl1_fn_4000   (file 0x04200, loads to seg-offset 0x4000)
;==============================================================================

ovl1_fn_4000:
  04000  55                 push   bp                      
  04001  8bec               mov    bp, sp                  
  04003  83ec06             sub    sp, 6                   
  04006  8b06dca6           mov    ax, word ptr [0xa6dc]   
  0400a  8946fc             mov    word ptr [bp - 4], ax   
  0400d  33c0               xor    ax, ax                  
  0400f  8906dca6           mov    word ptr [0xa6dc], ax   
  04013  b005               mov    al, 5                   
  04015  890692a6           mov    word ptr [0xa692], ax   
  04019  e88ef9             call   0x39aa   ; -> ovl1_fn_39aa
  0401c  8946fa             mov    word ptr [bp - 6], ax   
  0401f  0bc0               or     ax, ax                  
  04021  740a               je     0x402d                  
  04023  e875f8             call   0x389b   ; -> ovl1_fn_389b
  04026  8b46fa             mov    ax, word ptr [bp - 6]   
  04029  8be5               mov    sp, bp                  
  0402b  5d                 pop    bp                      
  0402c  c3                 ret                            
  0402d  ff36f5a6           push   word ptr [0xa6f5]       
  04031  e814c1             call   0x148                   
  04034  5f                 pop    di                      
  04035  83f801             cmp    ax, 1                   
  04038  7506               jne    0x4040                  
  0403a  33c0               xor    ax, ax                  
  0403c  8be5               mov    sp, bp                  
  0403e  5d                 pop    bp                      
  0403f  c3                 ret                            
  04040  813ecda80100       cmp    word ptr [0xa8cd], 1    
  04046  7c08               jl     0x4050                  
  04048  813e94a60100       cmp    word ptr [0xa694], 1    
  0404e  7d03               jge    0x4053                  
  04050  e9c200             jmp    0x4115                  
  04053  813e94a60b00       cmp    word ptr [0xa694], 0xb  
  04059  7d53               jge    0x40ae                  
  0405b  b8c108             mov    ax, 0x8c1               
  0405e  50                 push   ax                      
  0405f  bb0300             mov    bx, 3                   
  04062  8b0694a6           mov    ax, word ptr [0xa694]   
  04066  f7eb               imul   bx                      
  04068  5e                 pop    si                      
  04069  03f0               add    si, ax                  
  0406b  8976fe             mov    word ptr [bp - 2], si   
  0406e  b80300             mov    ax, 3                   
  04071  50                 push   ax                      
  04072  b83e0b             mov    ax, 0xb3e               
  04075  50                 push   ax                      
  04076  ff76fe             push   word ptr [bp - 2]       
  04079  e8c7de             call   0x1f43                  
  0407c  83c406             add    sp, 6                   
  0407f  bb0300             mov    bx, 3                   
  04082  b80b00             mov    ax, 0xb                 
  04085  2b0694a6           sub    ax, word ptr [0xa694]   
  04089  f7eb               imul   bx                      
  0408b  50                 push   ax                      
  0408c  ff76fe             push   word ptr [bp - 2]       
  0408f  8b46fe             mov    ax, word ptr [bp - 2]   
  04092  83c003             add    ax, 3                   
  04095  50                 push   ax                      
  04096  e8aade             call   0x1f43                  
  04099  83c406             add    sp, 6                   
  0409c  b80300             mov    ax, 3                   
  0409f  50                 push   ax                      
  040a0  b8e208             mov    ax, 0x8e2               
  040a3  50                 push   ax                      
  040a4  b83e0b             mov    ax, 0xb3e               
  040a7  50                 push   ax                      
  040a8  e898de             call   0x1f43                  
  040ab  83c406             add    sp, 6                   
  040ae  b8819a             mov    ax, 0x9a81              
  040b1  2b067aa6           sub    ax, word ptr [0xa67a]   
  040b5  33d2               xor    dx, dx                  
  040b7  8946fe             mov    word ptr [bp - 2], ax   
  040ba  817efee703         cmp    word ptr [bp - 2], 0x3e7
  040bf  7315               jae    0x40d6                  
  040c1  ff76fe             push   word ptr [bp - 2]       
  040c4  ff367aa6           push   word ptr [0xa67a]       
  040c8  8b067aa6           mov    ax, word ptr [0xa67a]   
  040cc  83c040             add    ax, 0x40                
  040cf  50                 push   ax                      
  040d0  e870de             call   0x1f43                  
  040d3  83c406             add    sp, 6                   
  040d6  b88297             mov    ax, 0x9782              
  040d9  50                 push   ax                      
  040da  bb4000             mov    bx, 0x40                
  040dd  8b06cda8           mov    ax, word ptr [0xa8cd]   
  040e1  f7eb               imul   bx                      
  040e3  83c05f             add    ax, 0x5f                
  040e6  5e                 pop    si                      
  040e7  03f0               add    si, ax                  
  040e9  8976fe             mov    word ptr [bp - 2], si   
  040ec  b82000             mov    ax, 0x20                
  040ef  50                 push   ax                      
  040f0  ff76fe             push   word ptr [bp - 2]       
  040f3  50                 push   ax                      
  040f4  e81ede             call   0x1f15                  
  040f7  83c406             add    sp, 6                   
  040fa  e82800             call   0x4125                  
  040fd  e85935             call   0x7659                  
  04100  e87d2d             call   0x6e80                  
  04103  ff0ecda8           dec    word ptr [0xa8cd]       
  04107  e8ff3c             call   0x7e09                  
  0410a  09c0               or     ax, ax                  
  0410c  7507               jne    0x4115                  
  0410e  b80e27             mov    ax, 0x270e              
  04111  8be5               mov    sp, bp                  
  04113  5d                 pop    bp                      
  04114  c3                 ret                            
  04115  e883f7             call   0x389b   ; -> ovl1_fn_389b
  04118  8b46fc             mov    ax, word ptr [bp - 4]   
  0411b  8906dca6           mov    word ptr [0xa6dc], ax   
  0411f  33c0               xor    ax, ax                  
  04121  8be5               mov    sp, bp                  
  04123  5d                 pop    bp                      
  04124  c3                 ret                            
  04125  55                 push   bp                      
  04126  8bec               mov    bp, sp                  
  04128  83ec02             sub    sp, 2                   
  0412b  33c0               xor    ax, ax                  
  0412d  8946fe             mov    word ptr [bp - 2], ax   
  04130  817efe2100         cmp    word ptr [bp - 2], 0x21 
  04135  7d26               jge    0x415d                  
  04137  b82000             mov    ax, 0x20                
  0413a  50                 push   ax                      
  0413b  e8892f             call   0x70c7                  
  0413e  5f                 pop    di                      
  0413f  8106c1a80200       add    word ptr [0xa8c1], 2    
  04145  ff36bea5           push   word ptr [0xa5be]       
  04149  e85dd6             call   0x17a9                  
  0414c  5f                 pop    di                      
  0414d  e87edf             call   0x20ce                  
  04150  b8f401             mov    ax, 0x1f4               
  04153  50                 push   ax                      
  04154  e831e0             call   0x2188                  
  04157  5f                 pop    di                      
  04158  ff46fe             inc    word ptr [bp - 2]       
  0415b  ebd3               jmp    0x4130                  
  0415d  8be5               mov    sp, bp                  
  0415f  5d                 pop    bp                      
  04160  c3                 ret                            
  04161  55                 push   bp                      
  04162  8bec               mov    bp, sp                  
  04164  83ec0a             sub    sp, 0xa                 
  04167  33c0               xor    ax, ax                  
  04169  8946fc             mov    word ptr [bp - 4], ax   
  0416c  8906cea5           mov    word ptr [0xa5ce], ax   
  04170  b8680b             mov    ax, 0xb68               
  04173  89066888           mov    word ptr [0x8868], ax   
  04177  b80200             mov    ax, 2                   
  0417a  8906dca6           mov    word ptr [0xa6dc], ax   

;==============================================================================
; OVERLAY 1 entry ovl1_fn_4808   (file 0x04A08, loads to seg-offset 0x4808)
;==============================================================================

ovl1_fn_4808:
  04808  55                 push   bp                      
  04809  8bec               mov    bp, sp                  
  0480b  83ec08             sub    sp, 8                   
  0480e  33c0               xor    ax, ax                  
  04810  8906cea5           mov    word ptr [0xa5ce], ax   
  04814  b8680b             mov    ax, 0xb68               
  04817  89066888           mov    word ptr [0x8868], ax   
  0481b  b8680b             mov    ax, 0xb68               
  0481e  05c05d             add    ax, 0x5dc0              
  04821  8906cea6           mov    word ptr [0xa6ce], ax   
  04825  b8680b             mov    ax, 0xb68               
  04828  50                 push   ax                      
  04829  b8b600             mov    ax, 0xb6                
  0482c  50                 push   ax                      
  0482d  e83339             call   0x8163                  
  04830  5f                 pop    di                      
  04831  5f                 pop    di                      
  04832  09c0               or     ax, ax                  
  04834  7507               jne    0x483d                  
  04836  b80f27             mov    ax, 0x270f              
  04839  8be5               mov    sp, bp                  
  0483b  5d                 pop    bp                      
  0483c  c3                 ret                            
  0483d  b8c800             mov    ax, 0xc8                
  04840  50                 push   ax                      
  04841  e865cf             call   0x17a9                  
  04844  5f                 pop    di                      
  04845  b80e00             mov    ax, 0xe                 
  04848  50                 push   ax                      
  04849  b00c               mov    al, 0xc                 
  0484b  50                 push   ax                      
  0484c  b002               mov    al, 2                   
  0484e  50                 push   ax                      
  0484f  33c0               xor    ax, ax                  
  04851  50                 push   ax                      
  04852  b00f               mov    al, 0xf                 
  04854  50                 push   ax                      
  04855  b00b               mov    al, 0xb                 
  04857  50                 push   ax                      
  04858  b007               mov    al, 7                   
  0485a  50                 push   ax                      
  0485b  33c0               xor    ax, ax                  
  0485d  50                 push   ax                      
  0485e  b00f               mov    al, 0xf                 
  04860  50                 push   ax                      
  04861  b009               mov    al, 9                   
  04863  50                 push   ax                      
  04864  b00a               mov    al, 0xa                 
  04866  50                 push   ax                      
  04867  33c0               xor    ax, ax                  
  04869  50                 push   ax                      
  0486a  e899bd             call   0x606                   
  0486d  83c418             add    sp, 0x18                
  04870  e821ed             call   0x3594                  
  04873  0bc0               or     ax, ax                  
  04875  7407               je     0x487e                  
  04877  b80f27             mov    ax, 0x270f              
  0487a  8be5               mov    sp, bp                  
  0487c  5d                 pop    bp                      
  0487d  c3                 ret                            
  0487e  8b06cea6           mov    ax, word ptr [0xa6ce]   
  04882  be82a6             mov    si, 0xa682              
  04885  8904               mov    word ptr [si], ax       
  04887  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0488b  83c044             add    ax, 0x44                
  0488e  be84a6             mov    si, 0xa684              
  04891  8904               mov    word ptr [si], ax       
  04893  8b06cea6           mov    ax, word ptr [0xa6ce]   
  04897  059000             add    ax, 0x90                
  0489a  be86a6             mov    si, 0xa686              
  0489d  8904               mov    word ptr [si], ax       
  0489f  8b06cea6           mov    ax, word ptr [0xa6ce]   
  048a3  05de00             add    ax, 0xde                
  048a6  be88a6             mov    si, 0xa688              
  048a9  8904               mov    word ptr [si], ax       
  048ab  8b06cea6           mov    ax, word ptr [0xa6ce]   
  048af  052101             add    ax, 0x121               
  048b2  be8aa6             mov    si, 0xa68a              
  048b5  8904               mov    word ptr [si], ax       
  048b7  8b06cea6           mov    ax, word ptr [0xa6ce]   
  048bb  056901             add    ax, 0x169               
  048be  be8ca6             mov    si, 0xa68c              
  048c1  8904               mov    word ptr [si], ax       
  048c3  8b06cea6           mov    ax, word ptr [0xa6ce]   
  048c7  05b701             add    ax, 0x1b7               
  048ca  be8ea6             mov    si, 0xa68e              
  048cd  8904               mov    word ptr [si], ax       
  048cf  8b06cea6           mov    ax, word ptr [0xa6ce]   
  048d3  050002             add    ax, 0x200               
  048d6  be90a6             mov    si, 0xa690              
  048d9  8904               mov    word ptr [si], ax       
  048db  8b06cea6           mov    ax, word ptr [0xa6ce]   
  048df  054702             add    ax, 0x247               
  048e2  890629a9           mov    word ptr [0xa929], ax   
  048e6  8b06cea6           mov    ax, word ptr [0xa6ce]   
  048ea  055702             add    ax, 0x257               
  048ed  89062ba9           mov    word ptr [0xa92b], ax   
  048f1  8b06cea6           mov    ax, word ptr [0xa6ce]   
  048f5  056702             add    ax, 0x267               
  048f8  89062da9           mov    word ptr [0xa92d], ax   
  048fc  8b06cea6           mov    ax, word ptr [0xa6ce]   
  04900  057702             add    ax, 0x277               
  04903  be31a9             mov    si, 0xa931              
  04906  8904               mov    word ptr [si], ax       
  04908  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0490c  059d02             add    ax, 0x29d               
  0490f  be33a9             mov    si, 0xa933              
  04912  8904               mov    word ptr [si], ax       
  04914  8b06cea6           mov    ax, word ptr [0xa6ce]   
  04918  05c302             add    ax, 0x2c3               
  0491b  be35a9             mov    si, 0xa935              
  0491e  8904               mov    word ptr [si], ax       
  04920  8b06cea6           mov    ax, word ptr [0xa6ce]   
  04924  05e902             add    ax, 0x2e9               
  04927  be37a9             mov    si, 0xa937              
  0492a  8904               mov    word ptr [si], ax       
  0492c  8b06cea6           mov    ax, word ptr [0xa6ce]   
  04930  050f03             add    ax, 0x30f               
  04933  be39a9             mov    si, 0xa939              
  04936  8904               mov    word ptr [si], ax       
  04938  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0493c  053503             add    ax, 0x335               
  0493f  be3ba9             mov    si, 0xa93b              
  04942  8904               mov    word ptr [si], ax       
  04944  8b06cea6           mov    ax, word ptr [0xa6ce]   
  04948  055b03             add    ax, 0x35b               
  0494b  be3da9             mov    si, 0xa93d              
  0494e  8904               mov    word ptr [si], ax       
  04950  8b06cea6           mov    ax, word ptr [0xa6ce]   
  04954  058103             add    ax, 0x381               
  04957  be3fa9             mov    si, 0xa93f              
  0495a  8904               mov    word ptr [si], ax       
  0495c  33c0               xor    ax, ax                  
  0495e  50                 push   ax                      
  0495f  e87cd5             call   0x1ede                  
  04962  5f                 pop    di                      
  04963  813e92a60900       cmp    word ptr [0xa692], 9    
  04969  7552               jne    0x49bd                  
  0496b  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0496f  05a703             add    ax, 0x3a7               
  04972  50                 push   ax                      
  04973  e8efdb             call   0x2565                  
  04976  5f                 pop    di                      
  04977  b80100             mov    ax, 1                   
  0497a  50                 push   ax                      
  0497b  e860d5             call   0x1ede                  
  0497e  5f                 pop    di                      
  0497f  8b06cea6           mov    ax, word ptr [0xa6ce]   
  04983  05cd03             add    ax, 0x3cd               
  04986  50                 push   ax                      

;==============================================================================
; OVERLAY 1 entry ovl1_fn_4eb2   (file 0x050B2, loads to seg-offset 0x4EB2)
;==============================================================================

ovl1_fn_4eb2:
  04eb2  55                 push   bp                      
  04eb3  8bec               mov    bp, sp                  
  04eb5  83ec02             sub    sp, 2                   
  04eb8  e83701             call   0x4ff2                  
  04ebb  33c0               xor    ax, ax                  
  04ebd  89067ca6           mov    word ptr [0xa67c], ax   
  04ec1  8906dca6           mov    word ptr [0xa6dc], ax   
  04ec5  b009               mov    al, 9                   
  04ec7  890692a6           mov    word ptr [0xa692], ax   
  04ecb  e86f2c             call   0x7b3d                  
  04ece  8946fe             mov    word ptr [bp - 2], ax   
  04ed1  817efe0000         cmp    word ptr [bp - 2], 0    
  04ed6  7507               jne    0x4edf                  
  04ed8  b80500             mov    ax, 5                   
  04edb  8be5               mov    sp, bp                  
  04edd  5d                 pop    bp                      
  04ede  c3                 ret                            
  04edf  817efe0500         cmp    word ptr [bp - 2], 5    
  04ee4  7514               jne    0x4efa                  
  04ee6  33c0               xor    ax, ax                  
  04ee8  50                 push   ax                      
  04ee9  e814b5             call   0x400                   
  04eec  5f                 pop    di                      
  04eed  0bc0               or     ax, ax                  
  04eef  7502               jne    0x4ef3                  
  04ef1  ebc8               jmp    0x4ebb                  
  04ef3  b80500             mov    ax, 5                   
  04ef6  8be5               mov    sp, bp                  
  04ef8  5d                 pop    bp                      
  04ef9  c3                 ret                            
  04efa  e89ee9             call   0x389b   ; -> ovl1_fn_389b
  04efd  817efe0600         cmp    word ptr [bp - 2], 6    
  04f02  750a               jne    0x4f0e                  
  04f04  e8f612             call   0x61fd                  
  04f07  b80600             mov    ax, 6                   
  04f0a  8be5               mov    sp, bp                  
  04f0c  5d                 pop    bp                      
  04f0d  c3                 ret                            
  04f0e  e8c025             call   0x74d1                  
  04f11  ff06cda8           inc    word ptr [0xa8cd]       
  04f15  8b06cda8           mov    ax, word ptr [0xa8cd]   
  04f19  83f80b             cmp    ax, 0xb                 
  04f1c  7e29               jle    0x4f47                  
  04f1e  e8ebd3             call   0x230c                  
  04f21  b81800             mov    ax, 0x18                
  04f24  50                 push   ax                      
  04f25  e8b6cf             call   0x1ede                  
  04f28  5f                 pop    di                      
  04f29  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  04f2d  05eb09             add    ax, 0x9eb               
  04f30  50                 push   ax                      
  04f31  e831d6             call   0x2565                  
  04f34  5f                 pop    di                      
  04f35  b83075             mov    ax, 0x7530              
  04f38  50                 push   ax                      
  04f39  e84cd2             call   0x2188                  
  04f3c  5f                 pop    di                      
  04f3d  e85be9             call   0x389b   ; -> ovl1_fn_389b
  04f40  b80500             mov    ax, 5                   
  04f43  8be5               mov    sp, bp                  
  04f45  5d                 pop    bp                      
  04f46  c3                 ret                            
  04f47  8b06cda8           mov    ax, word ptr [0xa8cd]   
  04f4b  890694a6           mov    word ptr [0xa694], ax   
  04f4f  33c0               xor    ax, ax                  
  04f51  8be5               mov    sp, bp                  
  04f53  5d                 pop    bp                      
  04f54  c3                 ret                            
  04f55  55                 push   bp                      
  04f56  8bec               mov    bp, sp                  
  04f58  83ec02             sub    sp, 2                   
  04f5b  33c0               xor    ax, ax                  
  04f5d  8946fe             mov    word ptr [bp - 2], ax   
  04f60  817efe1400         cmp    word ptr [bp - 2], 0x14 
  04f65  7d67               jge    0x4fce                  
  04f67  b80f27             mov    ax, 0x270f              
  04f6a  be1ea5             mov    si, 0xa51e              
  04f6d  8b56fe             mov    dx, word ptr [bp - 2]   
  04f70  d1e2               shl    dx, 1                   
  04f72  03f2               add    si, dx                  
  04f74  8904               mov    word ptr [si], ax       
  04f76  beb09e             mov    si, 0x9eb0              
  04f79  8b56fe             mov    dx, word ptr [bp - 2]   
  04f7c  d1e2               shl    dx, 1                   
  04f7e  03f2               add    si, dx                  
  04f80  8904               mov    word ptr [si], ax       
  04f82  b80200             mov    ax, 2                   
  04f85  be889e             mov    si, 0x9e88              
  04f88  8b56fe             mov    dx, word ptr [bp - 2]   
  04f8b  d1e2               shl    dx, 1                   
  04f8d  03f2               add    si, dx                  
  04f8f  8904               mov    word ptr [si], ax       
  04f91  b8ffff             mov    ax, 0xffff              
  04f94  be609e             mov    si, 0x9e60              
  04f97  8b56fe             mov    dx, word ptr [bp - 2]   
  04f9a  d1e2               shl    dx, 1                   
  04f9c  03f2               add    si, dx                  
  04f9e  8904               mov    word ptr [si], ax       
  04fa0  b80100             mov    ax, 1                   
  04fa3  be289f             mov    si, 0x9f28              
  04fa6  0376fe             add    si, word ptr [bp - 2]   
  04fa9  8804               mov    byte ptr [si], al       
  04fab  b8ffff             mov    ax, 0xffff              
  04fae  be2ca0             mov    si, 0xa02c              
  04fb1  8b56fe             mov    dx, word ptr [bp - 2]   
  04fb4  d1e2               shl    dx, 1                   
  04fb6  03f2               add    si, dx                  
  04fb8  8904               mov    word ptr [si], ax       
  04fba  b8feff             mov    ax, 0xfffe              
  04fbd  be04a0             mov    si, 0xa004              
  04fc0  8b56fe             mov    dx, word ptr [bp - 2]   
  04fc3  d1e2               shl    dx, 1                   
  04fc5  03f2               add    si, dx                  
  04fc7  8904               mov    word ptr [si], ax       
  04fc9  ff46fe             inc    word ptr [bp - 2]       
  04fcc  eb92               jmp    0x4f60                  
  04fce  b8bc02             mov    ax, 0x2bc               
  04fd1  50                 push   ax                      
  04fd2  b8a09f             mov    ax, 0x9fa0              
  04fd5  50                 push   ax                      
  04fd6  33c0               xor    ax, ax                  
  04fd8  50                 push   ax                      
  04fd9  e839cf             call   0x1f15                  
  04fdc  83c406             add    sp, 6                   
  04fdf  b80a00             mov    ax, 0xa                 
  04fe2  50                 push   ax                      
  04fe3  b8649f             mov    ax, 0x9f64              
  04fe6  50                 push   ax                      
  04fe7  b8ffff             mov    ax, 0xffff              
  04fea  50                 push   ax                      
  04feb  e83ecf             call   0x1f2c                  
  04fee  8be5               mov    sp, bp                  
  04ff0  5d                 pop    bp                      
  04ff1  c3                 ret                            
  04ff2  55                 push   bp                      
  04ff3  8bec               mov    bp, sp                  
  04ff5  b80100             mov    ax, 1                   
  04ff8  890605a7           mov    word ptr [0xa705], ax   
  04ffc  33c0               xor    ax, ax                  
  04ffe  890607a7           mov    word ptr [0xa707], ax   
  05002  b002               mov    al, 2                   
  05004  890609a7           mov    word ptr [0xa709], ax   
  05008  b03c               mov    al, 0x3c                
  0500a  89060ba7           mov    word ptr [0xa70b], ax   
  0500e  b002               mov    al, 2                   
  05010  89060da7           mov    word ptr [0xa70d], ax   
  05014  b050               mov    al, 0x50                
  05016  89060fa7           mov    word ptr [0xa70f], ax   
  0501a  b014               mov    al, 0x14                
  0501c  890611a7           mov    word ptr [0xa711], ax   
  05020  b008               mov    al, 8                   
  05022  890613a7           mov    word ptr [0xa713], ax   
  05026  b006               mov    al, 6                   
  05028  890615a7           mov    word ptr [0xa715], ax   
  0502c  8be5               mov    sp, bp                  
  0502e  5d                 pop    bp                      
  0502f  c3                 ret                            
  05030  55                 push   bp                      

;==============================================================================
; OVERLAY 1 entry ovl1_fn_53ae   (file 0x055AE, loads to seg-offset 0x53AE)
;==============================================================================

ovl1_fn_53ae:
  053ae  55                 push   bp                      
  053af  8bec               mov    bp, sp                  
  053b1  83ec04             sub    sp, 4                   
  053b4  8b06d8a6           mov    ax, word ptr [0xa6d8]   
  053b8  0bc0               or     ax, ax                  
  053ba  7404               je     0x53c0                  
  053bc  8be5               mov    sp, bp                  
  053be  5d                 pop    bp                      
  053bf  c3                 ret                            
  053c0  b81800             mov    ax, 0x18                
  053c3  50                 push   ax                      
  053c4  e817cb             call   0x1ede                  
  053c7  5f                 pop    di                      
  053c8  b8680b             mov    ax, 0xb68               
  053cb  89066888           mov    word ptr [0x8868], ax   
  053cf  b8680b             mov    ax, 0xb68               
  053d2  05401f             add    ax, 0x1f40              
  053d5  8906cea6           mov    word ptr [0xa6ce], ax   
  053d9  813ef7a60a00       cmp    word ptr [0xa6f7], 0xa  
  053df  7d1a               jge    0x53fb                  
  053e1  b8680b             mov    ax, 0xb68               
  053e4  50                 push   ax                      
  053e5  b8bc00             mov    ax, 0xbc                
  053e8  50                 push   ax                      
  053e9  e8772d             call   0x8163                  
  053ec  5f                 pop    di                      
  053ed  5f                 pop    di                      
  053ee  09c0               or     ax, ax                  
  053f0  7507               jne    0x53f9                  
  053f2  b80f27             mov    ax, 0x270f              
  053f5  8be5               mov    sp, bp                  
  053f7  5d                 pop    bp                      
  053f8  c3                 ret                            
  053f9  eb18               jmp    0x5413                  
  053fb  b8680b             mov    ax, 0xb68               
  053fe  50                 push   ax                      
  053ff  b8bf00             mov    ax, 0xbf                
  05402  50                 push   ax                      
  05403  e85d2d             call   0x8163                  
  05406  5f                 pop    di                      
  05407  5f                 pop    di                      
  05408  09c0               or     ax, ax                  
  0540a  7507               jne    0x5413                  
  0540c  b80f27             mov    ax, 0x270f              
  0540f  8be5               mov    sp, bp                  
  05411  5d                 pop    bp                      
  05412  c3                 ret                            
  05413  e851e1             call   0x3567                  
  05416  0bc0               or     ax, ax                  
  05418  7407               je     0x5421                  
  0541a  b80f27             mov    ax, 0x270f              
  0541d  8be5               mov    sp, bp                  
  0541f  5d                 pop    bp                      
  05420  c3                 ret                            
  05421  e870e1             call   0x3594                  
  05424  0bc0               or     ax, ax                  
  05426  7407               je     0x542f                  
  05428  b80f27             mov    ax, 0x270f              
  0542b  8be5               mov    sp, bp                  
  0542d  5d                 pop    bp                      
  0542e  c3                 ret                            
  0542f  b8680b             mov    ax, 0xb68               
  05432  056906             add    ax, 0x669               
  05435  8906c1a8           mov    word ptr [0xa8c1], ax   
  05439  ff36f9a6           push   word ptr [0xa6f9]       
  0543d  e8c602             call   0x5706                  
  05440  5f                 pop    di                      
  05441  b8680b             mov    ax, 0xb68               
  05444  057406             add    ax, 0x674               
  05447  8906c1a8           mov    word ptr [0xa8c1], ax   
  0544b  ff36fba6           push   word ptr [0xa6fb]       
  0544f  e8b402             call   0x5706                  
  05452  5f                 pop    di                      
  05453  b8680b             mov    ax, 0xb68               
  05456  054908             add    ax, 0x849               
  05459  8906c1a8           mov    word ptr [0xa8c1], ax   
  0545d  ff36fda6           push   word ptr [0xa6fd]       
  05461  e8a202             call   0x5706                  
  05464  5f                 pop    di                      
  05465  b8680b             mov    ax, 0xb68               
  05468  055408             add    ax, 0x854               
  0546b  8906c1a8           mov    word ptr [0xa8c1], ax   
  0546f  ff36ffa6           push   word ptr [0xa6ff]       
  05473  e89002             call   0x5706                  
  05476  5f                 pop    di                      
  05477  813e03a7e703       cmp    word ptr [0xa703], 0x3e7
  0547d  7f08               jg     0x5487                  
  0547f  813e03a70000       cmp    word ptr [0xa703], 0    
  05485  7d07               jge    0x548e                  
  05487  b8e703             mov    ax, 0x3e7               
  0548a  890603a7           mov    word ptr [0xa703], ax   
  0548e  813e01a7e703       cmp    word ptr [0xa701], 0x3e7
  05494  7f08               jg     0x549e                  
  05496  813e01a70000       cmp    word ptr [0xa701], 0    
  0549c  7d07               jge    0x54a5                  
  0549e  b8e703             mov    ax, 0x3e7               
  054a1  890601a7           mov    word ptr [0xa701], ax   
  054a5  b8680b             mov    ax, 0xb68               
  054a8  05790a             add    ax, 0xa79               
  054ab  8906c1a8           mov    word ptr [0xa8c1], ax   
  054af  ff3603a7           push   word ptr [0xa703]       
  054b3  e85002             call   0x5706                  
  054b6  5f                 pop    di                      
  054b7  b8680b             mov    ax, 0xb68               
  054ba  05840a             add    ax, 0xa84               
  054bd  8906c1a8           mov    word ptr [0xa8c1], ax   
  054c1  ff3601a7           push   word ptr [0xa701]       
  054c5  e83e02             call   0x5706                  
  054c8  5f                 pop    di                      
  054c9  b80100             mov    ax, 1                   
  054cc  8906c0a5           mov    word ptr [0xa5c0], ax   
  054d0  33c0               xor    ax, ax                  
  054d2  50                 push   ax                      
  054d3  e885b0             call   0x55b                   
  054d6  5f                 pop    di                      
  054d7  813ef7a60a00       cmp    word ptr [0xa6f7], 0xa  
  054dd  7d2d               jge    0x550c                  
  054df  b80f00             mov    ax, 0xf                 
  054e2  50                 push   ax                      
  054e3  b00b               mov    al, 0xb                 
  054e5  50                 push   ax                      
  054e6  b00a               mov    al, 0xa                 
  054e8  50                 push   ax                      
  054e9  33c0               xor    ax, ax                  
  054eb  50                 push   ax                      
  054ec  b00f               mov    al, 0xf                 
  054ee  50                 push   ax                      
  054ef  b00b               mov    al, 0xb                 
  054f1  50                 push   ax                      
  054f2  b007               mov    al, 7                   
  054f4  50                 push   ax                      
  054f5  33c0               xor    ax, ax                  
  054f7  50                 push   ax                      
  054f8  b00f               mov    al, 0xf                 
  054fa  50                 push   ax                      
  054fb  b00c               mov    al, 0xc                 
  054fd  50                 push   ax                      
  054fe  b003               mov    al, 3                   
  05500  50                 push   ax                      
  05501  33c0               xor    ax, ax                  
  05503  50                 push   ax                      
  05504  e8ffb0             call   0x606                   
  05507  83c418             add    sp, 0x18                
  0550a  eb2b               jmp    0x5537                  
  0550c  b80f00             mov    ax, 0xf                 
  0550f  50                 push   ax                      
  05510  b00c               mov    al, 0xc                 
  05512  50                 push   ax                      
  05513  b002               mov    al, 2                   
  05515  50                 push   ax                      
  05516  33c0               xor    ax, ax                  
  05518  50                 push   ax                      
  05519  b00f               mov    al, 0xf                 
  0551b  50                 push   ax                      
  0551c  b00b               mov    al, 0xb                 
  0551e  50                 push   ax                      
  0551f  b007               mov    al, 7                   
  05521  50                 push   ax                      
  05522  33c0               xor    ax, ax                  
  05524  50                 push   ax                      
  05525  b00f               mov    al, 0xf                 
  05527  50                 push   ax                      
  05528  b00c               mov    al, 0xc                 
  0552a  50                 push   ax                      
  0552b  b003               mov    al, 3                   
  0552d  50                 push   ax                      

;==============================================================================
; OVERLAY 1 entry ovl1_fn_5782   (file 0x05982, loads to seg-offset 0x5782)
;==============================================================================

ovl1_fn_5782:
  05782  55                 push   bp                      
  05783  8bec               mov    bp, sp                  
  05785  83ec08             sub    sp, 8                   
  05788  b80c00             mov    ax, 0xc                 
  0578b  890692a6           mov    word ptr [0xa692], ax   
  0578f  33c0               xor    ax, ax                  
  05791  8906cea5           mov    word ptr [0xa5ce], ax   
  05795  b8680b             mov    ax, 0xb68               
  05798  05401f             add    ax, 0x1f40              
  0579b  8906cea6           mov    word ptr [0xa6ce], ax   
  0579f  b8680b             mov    ax, 0xb68               
  057a2  50                 push   ax                      
  057a3  b8c600             mov    ax, 0xc6                
  057a6  50                 push   ax                      
  057a7  e8b929             call   0x8163                  
  057aa  5f                 pop    di                      
  057ab  5f                 pop    di                      
  057ac  09c0               or     ax, ax                  
  057ae  7507               jne    0x57b7                  
  057b0  b80f27             mov    ax, 0x270f              
  057b3  8be5               mov    sp, bp                  
  057b5  5d                 pop    bp                      
  057b6  c3                 ret                            
  057b7  e8dadd             call   0x3594                  
  057ba  0bc0               or     ax, ax                  
  057bc  7407               je     0x57c5                  
  057be  b80f27             mov    ax, 0x270f              
  057c1  8be5               mov    sp, bp                  
  057c3  5d                 pop    bp                      
  057c4  c3                 ret                            
  057c5  33c0               xor    ax, ax                  
  057c7  8946fc             mov    word ptr [bp - 4], ax   
  057ca  8946fe             mov    word ptr [bp - 2], ax   
  057cd  b8680b             mov    ax, 0xb68               
  057d0  051c09             add    ax, 0x91c               
  057d3  8906c1a8           mov    word ptr [0xa8c1], ax   
  057d7  8b06cea6           mov    ax, word ptr [0xa6ce]   
  057db  056313             add    ax, 0x1363              
  057de  50                 push   ax                      
  057df  e80216             call   0x6de4                  
  057e2  5f                 pop    di                      
  057e3  e88802             call   0x5a6e                  
  057e6  e8af03             call   0x5b98                  
  057e9  e86104             call   0x5c4d                  
  057ec  e80b03             call   0x5afa                  
  057ef  e80c04             call   0x5bfe                  
  057f2  b80200             mov    ax, 2                   
  057f5  8906c0a5           mov    word ptr [0xa5c0], ax   
  057f9  8906d0a5           mov    word ptr [0xa5d0], ax   
  057fd  33c0               xor    ax, ax                  
  057ff  50                 push   ax                      
  05800  e858ad             call   0x55b                   
  05803  5f                 pop    di                      
  05804  e89c04             call   0x5ca3                  
  05807  b80300             mov    ax, 3                   
  0580a  8906c5a8           mov    word ptr [0xa8c5], ax   
  0580e  33c0               xor    ax, ax                  
  05810  8906c3a8           mov    word ptr [0xa8c3], ax   
  05814  8b066888           mov    ax, word ptr [0x8868]   
  05818  05c40d             add    ax, 0xdc4               
  0581b  8906c7a8           mov    word ptr [0xa8c7], ax   
  0581f  8906c1a8           mov    word ptr [0xa8c1], ax   
  05823  b88297             mov    ax, 0x9782              
  05826  50                 push   ax                      
  05827  bb2000             mov    bx, 0x20                
  0582a  8b06c5a8           mov    ax, word ptr [0xa8c5]   
  0582e  f7eb               imul   bx                      
  05830  5e                 pop    si                      
  05831  03f0               add    si, ax                  
  05833  8b06c3a8           mov    ax, word ptr [0xa8c3]   
  05837  d1f8               sar    ax, 1                   
  05839  03f0               add    si, ax                  
  0583b  89367aa6           mov    word ptr [0xa67a], si   
  0583f  b80f00             mov    ax, 0xf                 
  05842  50                 push   ax                      
  05843  b00c               mov    al, 0xc                 
  05845  50                 push   ax                      
  05846  b002               mov    al, 2                   
  05848  50                 push   ax                      
  05849  33c0               xor    ax, ax                  
  0584b  50                 push   ax                      
  0584c  b00f               mov    al, 0xf                 
  0584e  50                 push   ax                      
  0584f  50                 push   ax                      
  05850  b007               mov    al, 7                   
  05852  50                 push   ax                      
  05853  33c0               xor    ax, ax                  
  05855  50                 push   ax                      
  05856  b007               mov    al, 7                   
  05858  50                 push   ax                      
  05859  b003               mov    al, 3                   
  0585b  50                 push   ax                      
  0585c  b006               mov    al, 6                   
  0585e  50                 push   ax                      
  0585f  33c0               xor    ax, ax                  
  05861  50                 push   ax                      
  05862  e8a1ad             call   0x606                   
  05865  83c418             add    sp, 0x18                
  05868  b80100             mov    ax, 1                   
  0586b  0bc0               or     ax, ax                  
  0586d  7503               jne    0x5872                  
  0586f  e9e101             jmp    0x5a53                  
  05872  b82800             mov    ax, 0x28                
  05875  50                 push   ax                      
  05876  8b066888           mov    ax, word ptr [0x8868]   
  0587a  ba680b             mov    dx, 0xb68               
  0587d  2bc2               sub    ax, dx                  
  0587f  33d2               xor    dx, dx                  
  05881  5b                 pop    bx                      
  05882  33d2               xor    dx, dx                  
  05884  f7f3               div    bx                      
  05886  8946f8             mov    word ptr [bp - 8], ax   
  05889  ff36bea5           push   word ptr [0xa5be]       
  0588d  e819bf             call   0x17a9                  
  05890  5f                 pop    di                      
  05891  bb0c00             mov    bx, 0xc                 
  05894  8b06c5a8           mov    ax, word ptr [0xa8c5]   
  05898  f7eb               imul   bx                      
  0589a  83e81a             sub    ax, 0x1a                
  0589d  2b46f8             sub    ax, word ptr [bp - 8]   
  058a0  50                 push   ax                      
  058a1  b80300             mov    ax, 3                   
  058a4  50                 push   ax                      
  058a5  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  058a9  05b408             add    ax, 0x8b4               
  058ac  50                 push   ax                      
  058ad  e88727             call   0x8037                  
  058b0  83c406             add    sp, 6                   
  058b3  ff36d0a5           push   word ptr [0xa5d0]       
  058b7  e8cec8             call   0x2188                  
  058ba  5f                 pop    di                      
  058bb  e887ec             call   0x4545                  
  058be  8946f8             mov    word ptr [bp - 8], ax   
  058c1  09c0               or     ax, ax                  
  058c3  750a               jne    0x58cf                  
  058c5  b8f401             mov    ax, 0x1f4               
  058c8  50                 push   ax                      
  058c9  e8bcc8             call   0x2188                  
  058cc  5f                 pop    di                      
  058cd  ebec               jmp    0x58bb                  
  058cf  817ef81c00         cmp    word ptr [bp - 8], 0x1c 
  058d4  7506               jne    0x58dc                  
  058d6  e8d0ac             call   0x5a9                   
  058d9  e97701             jmp    0x5a53                  
  058dc  ff76f8             push   word ptr [bp - 8]       
  058df  e8c0b6             call   0xfa2                   
  058e2  5f                 pop    di                      
  058e3  0bc0               or     ax, ax                  
  058e5  7429               je     0x5910                  
  058e7  ff4efe             dec    word ptr [bp - 2]       
  058ea  8b46fe             mov    ax, word ptr [bp - 2]   
  058ed  09c0               or     ax, ax                  
  058ef  7d07               jge    0x58f8                  
  058f1  33c0               xor    ax, ax                  
  058f3  8946fe             mov    word ptr [bp - 2], ax   
  058f6  eb06               jmp    0x58fe                  
  058f8  e8aeac             call   0x5a9                   
  058fb  e8e015             call   0x6ede                  
  058fe  8b06fa0a           mov    ax, word ptr [0xafa]    

;==============================================================================
; OVERLAY 1 entry ovl1_fn_7eda   (file 0x080DA, loads to seg-offset 0x7EDA)
;==============================================================================

ovl1_fn_7eda:
  07eda  fa                 cli                            
  07edb  06                 push   es                      
  07edc  33c0               xor    ax, ax                  
  07ede  a34505             mov    word ptr [0x545], ax    
  07ee1  a34305             mov    word ptr [0x543], ax    
  07ee4  a34105             mov    word ptr [0x541], ax    
  07ee7  8ec0               mov    es, ax                  
  07ee9  26a11400           mov    ax, word ptr es:[0x14]  
  07eed  a32705             mov    word ptr [0x527], ax    
  07ef0  26a11600           mov    ax, word ptr es:[0x16]  
  07ef4  a32905             mov    word ptr [0x529], ax    
  07ef7  8d060d22           lea    ax, [0x220d]            
  07efb  26a31400           mov    word ptr es:[0x14], ax  
  07eff  8cc8               mov    ax, cs                  
  07f01  26a31600           mov    word ptr es:[0x16], ax  
  07f05  26a16c00           mov    ax, word ptr es:[0x6c]  
  07f09  a32b05             mov    word ptr [0x52b], ax    
  07f0c  26a16e00           mov    ax, word ptr es:[0x6e]  
  07f10  a32d05             mov    word ptr [0x52d], ax    
  07f13  8d060d22           lea    ax, [0x220d]            
  07f17  26a36c00           mov    word ptr es:[0x6c], ax  
  07f1b  8cc8               mov    ax, cs                  
  07f1d  26a36e00           mov    word ptr es:[0x6e], ax  
  07f21  07                 pop    es                      
  07f22  fb                 sti                            
  07f23  cd11               int    0x11   ; BIOS equipment list
  07f25  a8c0               test   al, 0xc0                
  07f27  7406               je     0x7f2f                  
  07f29  c70645050100       mov    word ptr [0x545], 1     
  07f2f  f6c401             test   ah, 1                   
  07f32  7406               je     0x7f3a                  
  07f34  c70641050100       mov    word ptr [0x541], 1     
  07f3a  c70646090000       mov    word ptr [0x946], 0     
  07f40  a830               test   al, 0x30                
  07f42  7506               jne    0x7f4a                  
  07f44  c70646091810       mov    word ptr [0x946], 0x1018
  07f4a  1e                 push   ds                      
  07f4b  b800f0             mov    ax, 0xf000              
  07f4e  8ed8               mov    ds, ax                  
  07f50  a100c0             mov    ax, word ptr [0xc000]   
  07f53  3c21               cmp    al, 0x21                
  07f55  750a               jne    0x7f61                  
  07f57  1f                 pop    ds                      
  07f58  c70643050100       mov    word ptr [0x543], 1     
  07f5e  1e                 push   ds                      
  07f5f  eb15               jmp    0x7f76                  
  07f61  b84000             mov    ax, 0x40                
  07f64  8ed8               mov    ds, ax                  
  07f66  a11000             mov    ax, word ptr [0x10]     
  07f69  8bd8               mov    bx, ax                  
  07f6b  a92000             test   ax, 0x20                
  07f6e  7406               je     0x7f76                  
  07f70  25ef00             and    ax, 0xef                
  07f73  a31000             mov    word ptr [0x10], ax     
  07f76  1f                 pop    ds                      
  07f77  891e3905           mov    word ptr [0x539], bx    
  07f7b  b40f               mov    ah, 0xf                 
  07f7d  b000               mov    al, 0                   
  07f7f  cd10               int    0x10   ; BIOS video int10h AH=0f : get video mode
  07f81  b400               mov    ah, 0                   
  07f83  a33b05             mov    word ptr [0x53b], ax    
  07f86  e83005             call   0x84b9                  
  07f89  e81e00             call   0x7faa                  
  07f8c  e85605             call   0x84e5                  
  07f8f  1e                 push   ds                      
  07f90  ba0d22             mov    dx, 0x220d              
  07f93  0e                 push   cs                      
  07f94  1f                 pop    ds                      
  07f95  b023               mov    al, 0x23                
  07f97  b425               mov    ah, 0x25                
  07f99  cd21               int    0x21   ; DOS int21h AH=25 : set int vector
  07f9b  1f                 pop    ds                      
  07f9c  1e                 push   ds                      
  07f9d  babf21             mov    dx, 0x21bf              
  07fa0  0e                 push   cs                      
  07fa1  1f                 pop    ds                      
  07fa2  b024               mov    al, 0x24                
  07fa4  b425               mov    ah, 0x25                
  07fa6  cd21               int    0x21   ; DOS int21h AH=25 : set int vector
  07fa8  1f                 pop    ds                      
  07fa9  c3                 ret                            
  07faa  833efa0a00         cmp    word ptr [0xafa], 0     
  07faf  750b               jne    0x7fbc                  
  07fb1  bad403             mov    dx, 0x3d4               
  07fb4  b002               mov    al, 2                   
  07fb6  ee                 out    dx, al                  
  07fb7  42                 inc    dx                      
  07fb8  a0dea6             mov    al, byte ptr [0xa6de]   
  07fbb  ee                 out    dx, al                  
  07fbc  c3                 ret                            
  07fbd  55                 push   bp                      
  07fbe  8bec               mov    bp, sp                  
  07fc0  fc                 cld                            
  07fc1  8b7604             mov    si, word ptr [bp + 4]   
  07fc4  8b5e06             mov    bx, word ptr [bp + 6]   
  07fc7  d1e3               shl    bx, 1                   
  07fc9  06                 push   es                      
  07fca  a15909             mov    ax, word ptr [0x959]    
  07fcd  8ec0               mov    es, ax                  
  07fcf  8bc6               mov    ax, si                  
  07fd1  8b5608             mov    dx, word ptr [bp + 8]   
  07fd4  bd2800             mov    bp, 0x28                
  07fd7  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  07fdb  8bf0               mov    si, ax                  
  07fdd  8bcd               mov    cx, bp                  
  07fdf  f3a5               rep movsw word ptr es:[di], word ptr [si]
  07fe1  43                 inc    bx                      
  07fe2  43                 inc    bx                      
  07fe3  8bf0               mov    si, ax                  
  07fe5  81c6803e           add    si, 0x3e80              
  07fe9  8bbf6a09           mov    di, word ptr [bx + 0x96a]
  07fed  8bcd               mov    cx, bp                  
  07fef  f3a5               rep movsw word ptr es:[di], word ptr [si]
  07ff1  43                 inc    bx                      
  07ff2  43                 inc    bx                      
  07ff3  055000             add    ax, 0x50                
  07ff6  4a                 dec    dx                      
  07ff7  75de               jne    0x7fd7                  
  07ff9  07                 pop    es                      
  07ffa  5d                 pop    bp                      
  07ffb  c3                 ret                            
  07ffc  55                 push   bp                      
  07ffd  8bec               mov    bp, sp                  
  07fff  8b7604             mov    si, word ptr [bp + 4]   
  08002  8b7e06             mov    di, word ptr [bp + 6]   
  08005  893e4409           mov    word ptr [0x944], di    
  08009  06                 push   es                      
  0800a  8cd8               mov    ax, ds                  
  0800c  8ec0               mov    es, ax                  
  0800e  fc                 cld                            
  0800f  ad                 lodsw  ax, word ptr [si]       
  08010  8bd0               mov    dx, ax                  
  08012  ad                 lodsw  ax, word ptr [si]       
  08013  8bd8               mov    bx, ax                  
  08015  8b3e4409           mov    di, word ptr [0x944]    
  08019  8bca               mov    cx, dx                  
  0801b  f3a5               rep movsw word ptr es:[di], word ptr [si]
  0801d  8bca               mov    cx, dx                  
  0801f  8b3e4409           mov    di, word ptr [0x944]    
  08023  81c7803e           add    di, 0x3e80              
  08027  f3a5               rep movsw word ptr es:[di], word ptr [si]
  08029  8306440950         add    word ptr [0x944], 0x50  
  0802e  4b                 dec    bx                      
  0802f  83fb00             cmp    bx, 0                   
  08032  75e1               jne    0x8015                  
  08034  07                 pop    es                      
  08035  5d                 pop    bp                      
  08036  c3                 ret                            
  08037  55                 push   bp                      
  08038  8bec               mov    bp, sp                  
  0803a  8b7604             mov    si, word ptr [bp + 4]   
  0803d  8b5e08             mov    bx, word ptr [bp + 8]   
  08040  d1e3               shl    bx, 1                   
  08042  8b5606             mov    dx, word ptr [bp + 6]   
  08045  06                 push   es                      
  08046  a15909             mov    ax, word ptr [0x959]    
  08049  8ec0               mov    es, ax                  
  0804b  fc                 cld                            
  0804c  ad                 lodsw  ax, word ptr [si]       
  0804d  a34409             mov    word ptr [0x944], ax    
  08050  ad                 lodsw  ax, word ptr [si]       
  08051  d1e0               shl    ax, 1                   
  08053  a34609             mov    word ptr [0x946], ax    
  08056  e8fc9f             call   0x2055                  

;==============================================================================
; OVERLAY 2 entry ovl2_fn_8520   (file 0x08720, loads to seg-offset 0x8520)
;==============================================================================

ovl2_fn_8520:
  08520  55                 push   bp                      
  08521  8bec               mov    bp, sp                  
  08523  83ec08             sub    sp, 8                   
  08526  b88813             mov    ax, 0x1388              
  08529  50                 push   ax                      
  0852a  b86c88             mov    ax, 0x886c              
  0852d  50                 push   ax                      
  0852e  b86a01             mov    ax, 0x16a               
  08531  50                 push   ax                      
  08532  e8b892             call   0x17ed                  
  08535  83c406             add    sp, 6                   
  08538  09c0               or     ax, ax                  
  0853a  7507               jne    0x8543                  
  0853c  33c0               xor    ax, ax                  
  0853e  50                 push   ax                      
  0853f  e894a1             call   0x26d6                  
  08542  5f                 pop    di                      
  08543  b86c88             mov    ax, 0x886c              
  08546  8946f8             mov    word ptr [bp - 8], ax   
  08549  33c0               xor    ax, ax                  
  0854b  8946fa             mov    word ptr [bp - 6], ax   
  0854e  817efa8813         cmp    word ptr [bp - 6], 0x1388
  08553  7d35               jge    0x858a                  
  08555  8b76f8             mov    si, word ptr [bp - 8]   
  08558  0376fa             add    si, word ptr [bp - 6]   
  0855b  8a04               mov    al, byte ptr [si]       
  0855d  98                 cwde                           
  0855e  09c0               or     ax, ax                  
  08560  7523               jne    0x8585                  
  08562  8b76f8             mov    si, word ptr [bp - 8]   
  08565  8b46fa             mov    ax, word ptr [bp - 6]   
  08568  40                 inc    ax                      
  08569  03f0               add    si, ax                  
  0856b  8a04               mov    al, byte ptr [si]       
  0856d  98                 cwde                           
  0856e  09c0               or     ax, ax                  
  08570  7513               jne    0x8585                  
  08572  8b76f8             mov    si, word ptr [bp - 8]   
  08575  8b46fa             mov    ax, word ptr [bp - 6]   
  08578  ffc0               inc    ax                      
  0857a  ffc0               inc    ax                      
  0857c  03f0               add    si, ax                  
  0857e  8a04               mov    al, byte ptr [si]       
  08580  98                 cwde                           
  08581  09c0               or     ax, ax                  
  08583  7405               je     0x858a                  
  08585  ff46fa             inc    word ptr [bp - 6]       
  08588  ebc4               jmp    0x854e                  
  0858a  b87500             mov    ax, 0x75                
  0858d  8846fe             mov    byte ptr [bp - 2], al   
  08590  8b46fa             mov    ax, word ptr [bp - 6]   
  08593  8946fc             mov    word ptr [bp - 4], ax   
  08596  817efc0000         cmp    word ptr [bp - 4], 0    
  0859b  7e29               jle    0x85c6                  
  0859d  8b76f8             mov    si, word ptr [bp - 8]   
  085a0  8a04               mov    al, byte ptr [si]       
  085a2  98                 cwde                           
  085a3  50                 push   ax                      
  085a4  8a46fe             mov    al, byte ptr [bp - 2]   
  085a7  98                 cwde                           
  085a8  5a                 pop    dx                      
  085a9  33d0               xor    dx, ax                  
  085ab  8b76f8             mov    si, word ptr [bp - 8]   
  085ae  8814               mov    byte ptr [si], dl       
  085b0  ff46f8             inc    word ptr [bp - 8]       
  085b3  b81700             mov    ax, 0x17                
  085b6  50                 push   ax                      
  085b7  5a                 pop    dx                      
  085b8  8a46fe             mov    al, byte ptr [bp - 2]   
  085bb  98                 cwde                           
  085bc  03c2               add    ax, dx                  
  085be  8846fe             mov    byte ptr [bp - 2], al   
  085c1  ff4efc             dec    word ptr [bp - 4]       
  085c4  ebd0               jmp    0x8596                  
  085c6  e8dd03             call   0x89a6                  
  085c9  e8ca9a             call   0x2096                  
  085cc  e8db9a             call   0x20aa                  
  085cf  3b06dbaa           cmp    ax, word ptr [0xaadb]   
  085d3  7df7               jge    0x85cc                  
  085d5  b8680b             mov    ax, 0xb68               
  085d8  05401f             add    ax, 0x1f40              
  085db  8906cea6           mov    word ptr [0xa6ce], ax   
  085df  ff363505           push   word ptr [0x535]        
  085e3  e82000             call   0x8606                  
  085e6  5f                 pop    di                      
  085e7  8946fa             mov    word ptr [bp - 6], ax   
  085ea  0bc0               or     ax, ax                  
  085ec  750d               jne    0x85fb                  
  085ee  e8919c             call   0x2282                  
  085f1  e85b9d             call   0x234f                  
  085f4  33c0               xor    ax, ax                  
  085f6  50                 push   ax                      
  085f7  e8dca0             call   0x26d6                  
  085fa  5f                 pop    di                      
  085fb  b8680b             mov    ax, 0xb68               
  085fe  89066888           mov    word ptr [0x8868], ax   
  08602  8be5               mov    sp, bp                  
  08604  5d                 pop    bp                      
  08605  c3                 ret                            
  08606  55                 push   bp                      
  08607  8bec               mov    bp, sp                  
  08609  83ec04             sub    sp, 4                   
  0860c  b8680b             mov    ax, 0xb68               
  0860f  8906d8aa           mov    word ptr [0xaad8], ax   
  08613  8b4604             mov    ax, word ptr [bp + 4]   
  08616  0bc0               or     ax, ax                  
  08618  7415               je     0x862f                  
  0861a  8b36d8aa           mov    si, word ptr [0xaad8]   
  0861e  ff06d8aa           inc    word ptr [0xaad8]       
  08622  8a04               mov    al, byte ptr [si]       
  08624  98                 cwde                           
  08625  83f83b             cmp    ax, 0x3b                
  08628  75f0               jne    0x861a                  
  0862a  ff4e04             dec    word ptr [bp + 4]       
  0862d  ebe4               jmp    0x8613                  
  0862f  33c0               xor    ax, ax                  
  08631  8946fc             mov    word ptr [bp - 4], ax   
  08634  817efcc800         cmp    word ptr [bp - 4], 0xc8 
  08639  7d24               jge    0x865f                  
  0863b  8b36d8aa           mov    si, word ptr [0xaad8]   
  0863f  8a04               mov    al, byte ptr [si]       
  08641  98                 cwde                           
  08642  83f83f             cmp    ax, 0x3f                
  08645  7418               je     0x865f                  
  08647  8b36d8aa           mov    si, word ptr [0xaad8]   
  0864b  ff06d8aa           inc    word ptr [0xaad8]       
  0864f  8a04               mov    al, byte ptr [si]       
  08651  98                 cwde                           
  08652  beaca9             mov    si, 0xa9ac              
  08655  0376fc             add    si, word ptr [bp - 4]   
  08658  8804               mov    byte ptr [si], al       
  0865a  ff46fc             inc    word ptr [bp - 4]       
  0865d  ebd5               jmp    0x8634                  
  0865f  b83f00             mov    ax, 0x3f                
  08662  beaca9             mov    si, 0xa9ac              
  08665  8b56fc             mov    dx, word ptr [bp - 4]   
  08668  ff46fc             inc    word ptr [bp - 4]       
  0866b  03f2               add    si, dx                  
  0866d  8804               mov    byte ptr [si], al       
  0866f  33c0               xor    ax, ax                  
  08671  beaca9             mov    si, 0xa9ac              
  08674  0376fc             add    si, word ptr [bp - 4]   
  08677  8804               mov    byte ptr [si], al       
  08679  ff06d8aa           inc    word ptr [0xaad8]       
  0867d  33c0               xor    ax, ax                  
  0867f  8946fc             mov    word ptr [bp - 4], ax   
  08682  817efc3200         cmp    word ptr [bp - 4], 0x32 
  08687  7d24               jge    0x86ad                  
  08689  8b36d8aa           mov    si, word ptr [0xaad8]   
  0868d  8a04               mov    al, byte ptr [si]       
  0868f  98                 cwde                           
  08690  83f83b             cmp    ax, 0x3b                
  08693  7418               je     0x86ad                  
  08695  8b36d8aa           mov    si, word ptr [0xaad8]   
  08699  ff06d8aa           inc    word ptr [0xaad8]       
  0869d  8a04               mov    al, byte ptr [si]       
  0869f  98                 cwde                           

;==============================================================================
; OVERLAY 3 entry ovl3_fn_30f0   (file 0x08D00, loads to seg-offset 0x30F0)
;==============================================================================

ovl3_fn_30f0:
  030f0  55                 push   bp                      
  030f1  8bec               mov    bp, sp                  
  030f3  83ec0a             sub    sp, 0xa                 
  030f6  8b060fa7           mov    ax, word ptr [0xa70f]   
  030fa  8906cfa8           mov    word ptr [0xa8cf], ax   
  030fe  b85000             mov    ax, 0x50                
  03101  89060fa7           mov    word ptr [0xa70f], ax   
  03105  33c0               xor    ax, ax                  
  03107  89063105           mov    word ptr [0x531], ax    
  0310b  89062f05           mov    word ptr [0x52f], ax    
  0310f  8906cba8           mov    word ptr [0xa8cb], ax   
  03113  8906c9a8           mov    word ptr [0xa8c9], ax   
  03117  8906d4a6           mov    word ptr [0xa6d4], ax   
  0311b  8906d2a6           mov    word ptr [0xa6d2], ax   
  0311f  b8782d             mov    ax, 0x2d78              
  03122  89066888           mov    word ptr [0x8868], ax   
  03126  b86d00             mov    ax, 0x6d                
  03129  89066a88           mov    word ptr [0x886a], ax   
  0312d  b86c88             mov    ax, 0x886c              
  03130  8906cea6           mov    word ptr [0xa6ce], ax   
  03134  b8e803             mov    ax, 0x3e8               
  03137  50                 push   ax                      
  03138  ff36cea6           push   word ptr [0xa6ce]       
  0313c  b8c001             mov    ax, 0x1c0               
  0313f  50                 push   ax                      
  03140  e8aae6             call   0x17ed                  
  03143  83c406             add    sp, 6                   
  03146  09c0               or     ax, ax                  
  03148  7507               jne    0x3151                  
  0314a  b80f27             mov    ax, 0x270f              
  0314d  8be5               mov    sp, bp                  
  0314f  5d                 pop    bp                      
  03150  c3                 ret                            
  03151  b82600             mov    ax, 0x26                
  03154  89064a9e           mov    word ptr [0x9e4a], ax   
  03158  b091               mov    al, 0x91                
  0315a  89064c9e           mov    word ptr [0x9e4c], ax   
  0315e  b8de38             mov    ax, 0x38de              
  03161  89064e9e           mov    word ptr [0x9e4e], ax   
  03165  b8a401             mov    ax, 0x1a4               
  03168  50                 push   ax                      
  03169  b8b8a0             mov    ax, 0xa0b8              
  0316c  50                 push   ax                      
  0316d  33c0               xor    ax, ax                  
  0316f  50                 push   ax                      
  03170  e8a2ed             call   0x1f15                  
  03173  83c406             add    sp, 6                   
  03176  e83002             call   0x33a9                  
  03179  e8abd9             call   0xb27                   
  0317c  e8e9d2             call   0x468                   
  0317f  0bc0               or     ax, ax                  
  03181  7409               je     0x318c                  
  03183  b80f27             mov    ax, 0x270f              
  03186  8946f7             mov    word ptr [bp - 9], ax   
  03189  e90202             jmp    0x338e                  
  0318c  b80200             mov    ax, 2                   
  0318f  8906c0a5           mov    word ptr [0xa5c0], ax   
  03193  8906d0a5           mov    word ptr [0xa5d0], ax   
  03197  33c0               xor    ax, ax                  
  03199  50                 push   ax                      
  0319a  e8bed3             call   0x55b                   
  0319d  5f                 pop    di                      
  0319e  b80f00             mov    ax, 0xf                 
  031a1  50                 push   ax                      
  031a2  b009               mov    al, 9                   
  031a4  50                 push   ax                      
  031a5  b00a               mov    al, 0xa                 
  031a7  50                 push   ax                      
  031a8  33c0               xor    ax, ax                  
  031aa  50                 push   ax                      
  031ab  b00f               mov    al, 0xf                 
  031ad  50                 push   ax                      
  031ae  b00b               mov    al, 0xb                 
  031b0  50                 push   ax                      
  031b1  b00f               mov    al, 0xf                 
  031b3  50                 push   ax                      
  031b4  33c0               xor    ax, ax                  
  031b6  50                 push   ax                      
  031b7  b00f               mov    al, 0xf                 
  031b9  50                 push   ax                      
  031ba  b009               mov    al, 9                   
  031bc  50                 push   ax                      
  031bd  b00a               mov    al, 0xa                 
  031bf  50                 push   ax                      
  031c0  33c0               xor    ax, ax                  
  031c2  50                 push   ax                      
  031c3  e840d4             call   0x606                   
  031c6  83c418             add    sp, 0x18                
  031c9  e8b4dd             call   0xf80                   
  031cc  813e92a60900       cmp    word ptr [0xa692], 9    
  031d2  753f               jne    0x3213                  
  031d4  ff36bea5           push   word ptr [0xa5be]       
  031d8  e8cee5             call   0x17a9                  
  031db  5f                 pop    di                      
  031dc  ff36cea6           push   word ptr [0xa6ce]       
  031e0  e882f3             call   0x2565                  
  031e3  5f                 pop    di                      
  031e4  b81800             mov    ax, 0x18                
  031e7  50                 push   ax                      
  031e8  e8f3ec             call   0x1ede                  
  031eb  5f                 pop    di                      
  031ec  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031f0  05ea01             add    ax, 0x1ea               
  031f3  50                 push   ax                      
  031f4  e86ef3             call   0x2565                  
  031f7  5f                 pop    di                      
  031f8  813e2f050000       cmp    word ptr [0x52f], 0     
  031fe  74f8               je     0x31f8                  
  03200  e8a6d3             call   0x5a9                   
  03203  b81700             mov    ax, 0x17                
  03206  50                 push   ax                      
  03207  e8d4ec             call   0x1ede                  
  0320a  5f                 pop    di                      
  0320b  b8581b             mov    ax, 0x1b58              
  0320e  50                 push   ax                      
  0320f  e876ef             call   0x2188                  
  03212  5f                 pop    di                      
  03213  e8f801             call   0x340e                  
  03216  33c0               xor    ax, ax                  
  03218  8946fd             mov    word ptr [bp - 3], ax   
  0321b  8946fb             mov    word ptr [bp - 5], ax   
  0321e  8946f9             mov    word ptr [bp - 7], ax   
  03221  b80100             mov    ax, 1                   
  03224  0bc0               or     ax, ax                  
  03226  7503               jne    0x322b                  
  03228  e9e700             jmp    0x3312                  
  0322b  ff36bea5           push   word ptr [0xa5be]       
  0322f  e877e5             call   0x17a9                  
  03232  5f                 pop    di                      
  03233  ff36d0a5           push   word ptr [0xa5d0]       
  03237  e84eef             call   0x2188                  
  0323a  5f                 pop    di                      
  0323b  e81907             call   0x3957                  
  0323e  0bc0               or     ax, ax                  
  03240  7503               jne    0x3245                  
  03242  e9ca00             jmp    0x330f                  
  03245  e861d3             call   0x5a9                   
  03248  b81800             mov    ax, 0x18                
  0324b  50                 push   ax                      
  0324c  e88fec             call   0x1ede                  
  0324f  5f                 pop    di                      
  03250  813e92a60000       cmp    word ptr [0xa692], 0    
  03256  755e               jne    0x32b6                  
  03258  b81800             mov    ax, 0x18                
  0325b  50                 push   ax                      
  0325c  e87fec             call   0x1ede                  

ovl1_fn_325f:
  0325f  5f                 pop    di                      
  03260  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03264  056f01             add    ax, 0x16f               
  03267  50                 push   ax                      
  03268  e8faf2             call   0x2565                  
  0326b  5f                 pop    di                      
  0326c  b8581b             mov    ax, 0x1b58              
  0326f  50                 push   ax                      

;==============================================================================
; OVERLAY 4 entry ovl4_fn_30f0   (file 0x0A7F0, loads to seg-offset 0x30F0)
;==============================================================================

ovl4_fn_30f0:
  030f0  55                 push   bp                      
  030f1  8bec               mov    bp, sp                  
  030f3  83ec08             sub    sp, 8                   
  030f6  b8680b             mov    ax, 0xb68               
  030f9  890629a9           mov    word ptr [0xa929], ax   
  030fd  33c0               xor    ax, ax                  
  030ff  8906c1a8           mov    word ptr [0xa8c1], ax   
  03103  8906cda8           mov    word ptr [0xa8cd], ax   
  03107  8906c5a8           mov    word ptr [0xa8c5], ax   
  0310b  8906c3a8           mov    word ptr [0xa8c3], ax   
  0310f  8906c7a8           mov    word ptr [0xa8c7], ax   
  03113  8906c9a8           mov    word ptr [0xa8c9], ax   
  03117  8906d2a6           mov    word ptr [0xa6d2], ax   
  0311b  89063105           mov    word ptr [0x531], ax    
  0311f  89062f05           mov    word ptr [0x52f], ax    
  03123  b8680b             mov    ax, 0xb68               
  03126  05803e             add    ax, 0x3e80              
  03129  8906cea6           mov    word ptr [0xa6ce], ax   
  0312d  b8401f             mov    ax, 0x1f40              
  03130  50                 push   ax                      
  03131  ff36cea6           push   word ptr [0xa6ce]       
  03135  b8c301             mov    ax, 0x1c3               
  03138  50                 push   ax                      
  03139  e8b1e6             call   0x17ed                  
  0313c  83c406             add    sp, 6                   
  0313f  09c0               or     ax, ax                  
  03141  7507               jne    0x314a                  
  03143  b80f27             mov    ax, 0x270f              
  03146  8be5               mov    sp, bp                  
  03148  5d                 pop    bp                      
  03149  c3                 ret                            
  0314a  e8b208             call   0x39ff                  
  0314d  0bc0               or     ax, ax                  
  0314f  7407               je     0x3158                  
  03151  b80f27             mov    ax, 0x270f              
  03154  8be5               mov    sp, bp                  
  03156  5d                 pop    bp                      
  03157  c3                 ret                            
  03158  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0315c  89062ba9           mov    word ptr [0xa92b], ax   
  03160  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03164  83c024             add    ax, 0x24                
  03167  89062da9           mov    word ptr [0xa92d], ax   
  0316b  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0316f  054b01             add    ax, 0x14b               
  03172  89069ba9           mov    word ptr [0xa99b], ax   
  03176  b81800             mov    ax, 0x18                
  03179  8946f9             mov    word ptr [bp - 7], ax   
  0317c  817ef90000         cmp    word ptr [bp - 7], 0    
  03181  7c0c               jl     0x318f                  
  03183  ff76f9             push   word ptr [bp - 7]       
  03186  e855ed             call   0x1ede                  
  03189  5f                 pop    di                      
  0318a  ff4ef9             dec    word ptr [bp - 7]       
  0318d  ebed               jmp    0x317c                  
  0318f  b80f00             mov    ax, 0xf                 
  03192  50                 push   ax                      
  03193  b009               mov    al, 9                   
  03195  50                 push   ax                      
  03196  b00a               mov    al, 0xa                 
  03198  50                 push   ax                      
  03199  33c0               xor    ax, ax                  
  0319b  50                 push   ax                      
  0319c  b00f               mov    al, 0xf                 
  0319e  50                 push   ax                      
  0319f  b00b               mov    al, 0xb                 
  031a1  50                 push   ax                      
  031a2  b00f               mov    al, 0xf                 
  031a4  50                 push   ax                      
  031a5  33c0               xor    ax, ax                  
  031a7  50                 push   ax                      
  031a8  b00f               mov    al, 0xf                 
  031aa  50                 push   ax                      
  031ab  b009               mov    al, 9                   
  031ad  50                 push   ax                      
  031ae  b00a               mov    al, 0xa                 
  031b0  50                 push   ax                      
  031b1  33c0               xor    ax, ax                  
  031b3  50                 push   ax                      
  031b4  e84fd4             call   0x606                   
  031b7  83c418             add    sp, 0x18                
  031ba  e83d02             call   0x33fa                  
  031bd  b80100             mov    ax, 1                   
  031c0  8906c0a5           mov    word ptr [0xa5c0], ax   
  031c4  33c0               xor    ax, ax                  
  031c6  50                 push   ax                      
  031c7  e891d3             call   0x55b                   
  031ca  5f                 pop    di                      
  031cb  813e92a60300       cmp    word ptr [0xa692], 3    
  031d1  7403               je     0x31d6                  
  031d3  e92201             jmp    0x32f8                  
  031d6  b80100             mov    ax, 1                   
  031d9  50                 push   ax                      
  031da  50                 push   ax                      
  031db  e87eee             call   0x205c                  
  031de  5f                 pop    di                      
  031df  5f                 pop    di                      
  031e0  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031e4  83c048             add    ax, 0x48                
  031e7  50                 push   ax                      
  031e8  e87af3             call   0x2565                  
  031eb  5f                 pop    di                      
  031ec  b80200             mov    ax, 2                   
  031ef  50                 push   ax                      
  031f0  b001               mov    al, 1                   
  031f2  50                 push   ax                      
  031f3  e866ee             call   0x205c                  
  031f6  5f                 pop    di                      
  031f7  5f                 pop    di                      
  031f8  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031fc  83c06f             add    ax, 0x6f                
  031ff  50                 push   ax                      
  03200  e862f3             call   0x2565                  
  03203  5f                 pop    di                      
  03204  b80500             mov    ax, 5                   
  03207  50                 push   ax                      
  03208  b001               mov    al, 1                   
  0320a  50                 push   ax                      
  0320b  e84eee             call   0x205c                  
  0320e  5f                 pop    di                      
  0320f  5f                 pop    di                      
  03210  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03214  05b100             add    ax, 0xb1                
  03217  50                 push   ax                      
  03218  e84af3             call   0x2565                  
  0321b  5f                 pop    di                      
  0321c  b80600             mov    ax, 6                   
  0321f  50                 push   ax                      
  03220  b001               mov    al, 1                   
  03222  50                 push   ax                      
  03223  e836ee             call   0x205c                  
  03226  5f                 pop    di                      
  03227  5f                 pop    di                      
  03228  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0322c  05d200             add    ax, 0xd2                
  0322f  50                 push   ax                      
  03230  e832f3             call   0x2565                  
  03233  5f                 pop    di                      
  03234  b82800             mov    ax, 0x28                
  03237  50                 push   ax                      
  03238  b00d               mov    al, 0xd                 
  0323a  50                 push   ax                      
  0323b  b82404             mov    ax, 0x424               
  0323e  50                 push   ax                      
  0323f  e818ed             call   0x1f5a                  
  03242  83c406             add    sp, 6                   
  03245  b82800             mov    ax, 0x28                
  03248  50                 push   ax                      
  03249  b029               mov    al, 0x29                
  0324b  50                 push   ax                      
  0324c  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03250  05f600             add    ax, 0xf6                
  03253  50                 push   ax                      
  03254  e803ed             call   0x1f5a                  
  03257  83c406             add    sp, 6                   
  0325a  b86300             mov    ax, 0x63                
  0325d  8906c0a5           mov    word ptr [0xa5c0], ax   
  03261  b80100             mov    ax, 1                   
  03264  0bc0               or     ax, ax                  
  03266  742c               je     0x3294                  
  03268  e88b05             call   0x37f6                  
  0326b  8846f8             mov    byte ptr [bp - 8], al   
  0326e  98                 cwde                           

;==============================================================================
; OVERLAY 5 entry ovl5_fn_30f0   (file 0x0B2C0, loads to seg-offset 0x30F0)
;==============================================================================

ovl5_fn_30f0:
  030f0  55                 push   bp                      
  030f1  8bec               mov    bp, sp                  
  030f3  83ec06             sub    sp, 6                   
  030f6  b82a00             mov    ax, 0x2a                
  030f9  89064a9e           mov    word ptr [0x9e4a], ax   
  030fd  b091               mov    al, 0x91                
  030ff  89064c9e           mov    word ptr [0x9e4c], ax   
  03103  b8a82a             mov    ax, 0x2aa8              
  03106  89066888           mov    word ptr [0x8868], ax   
  0310a  b8e238             mov    ax, 0x38e2              
  0310d  89064e9e           mov    word ptr [0x9e4e], ax   
  03111  b80600             mov    ax, 6                   
  03114  890606a5           mov    word ptr [0xa506], ax   
  03118  b8680b             mov    ax, 0xb68               
  0311b  05803e             add    ax, 0x3e80              
  0311e  8906cea6           mov    word ptr [0xa6ce], ax   
  03122  b8401f             mov    ax, 0x1f40              
  03125  50                 push   ax                      
  03126  ff36cea6           push   word ptr [0xa6ce]       
  0312a  b8c601             mov    ax, 0x1c6               
  0312d  50                 push   ax                      
  0312e  e8bce6             call   0x17ed                  
  03131  83c406             add    sp, 6                   
  03134  09c0               or     ax, ax                  
  03136  7509               jne    0x3141                  
  03138  b80f27             mov    ax, 0x270f              
  0313b  8946fb             mov    word ptr [bp - 5], ax   
  0313e  e96002             jmp    0x33a1                  
  03141  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03145  be33a9             mov    si, 0xa933              
  03148  8904               mov    word ptr [si], ax       
  0314a  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0314e  05ac00             add    ax, 0xac                
  03151  be35a9             mov    si, 0xa935              
  03154  8904               mov    word ptr [si], ax       
  03156  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0315a  052001             add    ax, 0x120               
  0315d  be37a9             mov    si, 0xa937              
  03160  8904               mov    word ptr [si], ax       
  03162  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03166  05cc01             add    ax, 0x1cc               
  03169  be39a9             mov    si, 0xa939              
  0316c  8904               mov    word ptr [si], ax       
  0316e  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03172  054002             add    ax, 0x240               
  03175  be3ba9             mov    si, 0xa93b              
  03178  8904               mov    word ptr [si], ax       
  0317a  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0317e  05ec02             add    ax, 0x2ec               
  03181  be3da9             mov    si, 0xa93d              
  03184  8904               mov    word ptr [si], ax       
  03186  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0318a  05eb05             add    ax, 0x5eb               
  0318d  890629a9           mov    word ptr [0xa929], ax   
  03191  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03195  059803             add    ax, 0x398               
  03198  becca0             mov    si, 0xa0cc              
  0319b  8904               mov    word ptr [si], ax       
  0319d  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031a1  051604             add    ax, 0x416               
  031a4  becea0             mov    si, 0xa0ce              
  031a7  8904               mov    word ptr [si], ax       
  031a9  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031ad  059404             add    ax, 0x494               
  031b0  bed0a0             mov    si, 0xa0d0              
  031b3  8904               mov    word ptr [si], ax       
  031b5  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031b9  05a603             add    ax, 0x3a6               
  031bc  bef4a0             mov    si, 0xa0f4              
  031bf  8904               mov    word ptr [si], ax       
  031c1  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031c5  052404             add    ax, 0x424               
  031c8  bef6a0             mov    si, 0xa0f6              
  031cb  8904               mov    word ptr [si], ax       
  031cd  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031d1  05a204             add    ax, 0x4a2               
  031d4  bef8a0             mov    si, 0xa0f8              
  031d7  8904               mov    word ptr [si], ax       
  031d9  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031dd  05b403             add    ax, 0x3b4               
  031e0  be1ca1             mov    si, 0xa11c              
  031e3  8904               mov    word ptr [si], ax       
  031e5  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031e9  053204             add    ax, 0x432               
  031ec  be1ea1             mov    si, 0xa11e              
  031ef  8904               mov    word ptr [si], ax       
  031f1  8b06cea6           mov    ax, word ptr [0xa6ce]   
  031f5  05b004             add    ax, 0x4b0               
  031f8  be20a1             mov    si, 0xa120              
  031fb  8904               mov    word ptr [si], ax       
  031fd  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03201  05c203             add    ax, 0x3c2               
  03204  be44a1             mov    si, 0xa144              
  03207  8904               mov    word ptr [si], ax       
  03209  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0320d  054004             add    ax, 0x440               
  03210  be46a1             mov    si, 0xa146              
  03213  8904               mov    word ptr [si], ax       
  03215  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03219  05be04             add    ax, 0x4be               
  0321c  be48a1             mov    si, 0xa148              
  0321f  8904               mov    word ptr [si], ax       
  03221  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03225  05d003             add    ax, 0x3d0               
  03228  be94a1             mov    si, 0xa194              
  0322b  8904               mov    word ptr [si], ax       
  0322d  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03231  054e04             add    ax, 0x44e               
  03234  be96a1             mov    si, 0xa196              
  03237  8904               mov    word ptr [si], ax       
  03239  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0323d  05cc04             add    ax, 0x4cc               
  03240  be98a1             mov    si, 0xa198              
  03243  8904               mov    word ptr [si], ax       
  03245  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03249  05de03             add    ax, 0x3de               
  0324c  bebca1             mov    si, 0xa1bc              
  0324f  8904               mov    word ptr [si], ax       
  03251  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03255  055c04             add    ax, 0x45c               
  03258  bebea1             mov    si, 0xa1be              
  0325b  8904               mov    word ptr [si], ax       
  0325d  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03261  05da04             add    ax, 0x4da               
  03264  bec0a1             mov    si, 0xa1c0              
  03267  8904               mov    word ptr [si], ax       
  03269  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0326d  05ec03             add    ax, 0x3ec               

;==============================================================================
; OVERLAY 6 entry ovl6_fn_30f0   (file 0x0C950, loads to seg-offset 0x30F0)
;==============================================================================

ovl6_fn_30f0:
  030f0  55                 push   bp                      
  030f1  8bec               mov    bp, sp                  
  030f3  83ec08             sub    sp, 8                   
  030f6  e82eda             call   0xb27                   
  030f9  e86cd3             call   0x468                   
  030fc  0bc0               or     ax, ax                  
  030fe  7407               je     0x3107                  
  03100  b80f27             mov    ax, 0x270f              
  03103  8be5               mov    sp, bp                  
  03105  5d                 pop    bp                      
  03106  c3                 ret                            
  03107  b87303             mov    ax, 0x373               
  0310a  50                 push   ax                      
  0310b  b82494             mov    ax, 0x9424              
  0310e  50                 push   ax                      
  0310f  b8cc01             mov    ax, 0x1cc               
  03112  50                 push   ax                      
  03113  e8d7e6             call   0x17ed                  
  03116  83c406             add    sp, 6                   
  03119  09c0               or     ax, ax                  
  0311b  7507               jne    0x3124                  
  0311d  b80f27             mov    ax, 0x270f              
  03120  8be5               mov    sp, bp                  
  03122  5d                 pop    bp                      
  03123  c3                 ret                            
  03124  b82494             mov    ax, 0x9424              
  03127  8906ddaa           mov    word ptr [0xaadd], ax   
  0312b  8b06ddaa           mov    ax, word ptr [0xaadd]   
  0312f  83c012             add    ax, 0x12                
  03132  8906dfaa           mov    word ptr [0xaadf], ax   
  03136  33c0               xor    ax, ax                  
  03138  89062fa9           mov    word ptr [0xa92f], ax   
  0313c  8906edaa           mov    word ptr [0xaaed], ax   
  03140  8906e7aa           mov    word ptr [0xaae7], ax   
  03144  e8cc02             call   0x3413                  
  03147  e8f055             call   0x873a                  
  0314a  ff360ca5           push   word ptr [0xa50c]       
  0314e  ff360ea5           push   word ptr [0xa50e]       
  03152  e83404             call   0x3589                  
  03155  5f                 pop    di                      
  03156  5f                 pop    di                      
  03157  8946f8             mov    word ptr [bp - 8], ax   
  0315a  0bc0               or     ax, ax                  
  0315c  7407               je     0x3165                  
  0315e  8b46f8             mov    ax, word ptr [bp - 8]   
  03161  8be5               mov    sp, bp                  
  03163  5d                 pop    bp                      
  03164  c3                 ret                            
  03165  e8165a             call   0x8b7e                  
  03168  8b06cca5           mov    ax, word ptr [0xa5cc]   
  0316c  0bc0               or     ax, ax                  
  0316e  7436               je     0x31a6                  
  03170  e8bc4c             call   0x7e2f                  
  03173  b8ffff             mov    ax, 0xffff              
  03176  890610a5           mov    word ptr [0xa510], ax   
  0317a  e86109             call   0x3ade                  
  0317d  e87607             call   0x38f6                  
  03180  33c0               xor    ax, ax                  
  03182  8946f8             mov    word ptr [bp - 8], ax   
  03185  817ef80500         cmp    word ptr [bp - 8], 5    
  0318a  7d08               jge    0x3194                  
  0318c  e80945             call   0x7698                  
  0318f  ff46f8             inc    word ptr [bp - 8]       
  03192  ebf1               jmp    0x3185                  
  03194  e85f07             call   0x38f6                  
  03197  8b06d8a6           mov    ax, word ptr [0xa6d8]   
  0319b  0bc0               or     ax, ax                  
  0319d  7407               je     0x31a6                  
  0319f  b84a00             mov    ax, 0x4a                
  031a2  8906edaa           mov    word ptr [0xaaed], ax   
  031a6  33c0               xor    ax, ax                  
  031a8  8906c2a5           mov    word ptr [0xa5c2], ax   
  031ac  8906e5aa           mov    word ptr [0xaae5], ax   
  031b0  b002               mov    al, 2                   
  031b2  8906c0a5           mov    word ptr [0xa5c0], ax   
  031b6  8906ebaa           mov    word ptr [0xaaeb], ax   
  031ba  8906e9aa           mov    word ptr [0xaae9], ax   
  031be  33c0               xor    ax, ax                  
  031c0  50                 push   ax                      
  031c1  e897d3             call   0x55b                   
  031c4  5f                 pop    di                      
  031c5  e8e439             call   0x6bac                  
  031c8  e8a62f             call   0x6171                  
  031cb  b80f00             mov    ax, 0xf                 
  031ce  50                 push   ax                      
  031cf  b009               mov    al, 9                   
  031d1  50                 push   ax                      
  031d2  b00a               mov    al, 0xa                 
  031d4  50                 push   ax                      
  031d5  33c0               xor    ax, ax                  
  031d7  50                 push   ax                      
  031d8  b00f               mov    al, 0xf                 
  031da  50                 push   ax                      
  031db  b007               mov    al, 7                   
  031dd  50                 push   ax                      
  031de  b00b               mov    al, 0xb                 
  031e0  50                 push   ax                      
  031e1  33c0               xor    ax, ax                  
  031e3  50                 push   ax                      
  031e4  b00f               mov    al, 0xf                 
  031e6  50                 push   ax                      
  031e7  b009               mov    al, 9                   
  031e9  50                 push   ax                      
  031ea  b00a               mov    al, 0xa                 
  031ec  50                 push   ax                      
  031ed  33c0               xor    ax, ax                  
  031ef  50                 push   ax                      
  031f0  e813d4             call   0x606                   
  031f3  83c418             add    sp, 0x18                
  031f6  8b06cca5           mov    ax, word ptr [0xa5cc]   
  031fa  0bc0               or     ax, ax                  
  031fc  7409               je     0x3207                  
  031fe  e8f506             call   0x38f6                  
  03201  33c0               xor    ax, ax                  
  03203  8906cca5           mov    word ptr [0xa5cc], ax   
  03207  b80100             mov    ax, 1                   
  0320a  0bc0               or     ax, ax                  
  0320c  7503               jne    0x3211                  
  0320e  e9fb01             jmp    0x340c                  
  03211  ff36bea5           push   word ptr [0xa5be]       
  03215  e891e5             call   0x17a9                  
  03218  5f                 pop    di                      
  03219  e8960f             call   0x41b2                  
  0321c  0bc0               or     ax, ax                  
  0321e  7511               jne    0x3231                  
  03220  e8031c             call   0x4e26                  
  03223  8946f8             mov    word ptr [bp - 8], ax   
  03226  0bc0               or     ax, ax                  
  03228  7407               je     0x3231                  
  0322a  8b46f8             mov    ax, word ptr [bp - 8]   
  0322d  8be5               mov    sp, bp                  
  0322f  5d                 pop    bp                      
  03230  c3                 ret                            
  03231  8b063105           mov    ax, word ptr [0x531]    
  03235  0bc0               or     ax, ax                  
  03237  740f               je     0x3248                  
  03239  8b06d8a6           mov    ax, word ptr [0xa6d8]   
  0323d  0bc0               or     ax, ax                  
  0323f  7407               je     0x3248                  
  03241  b80100             mov    ax, 1                   
  03244  8be5               mov    sp, bp                  
  03246  5d                 pop    bp                      
  03247  c3                 ret                            
  03248  ff061aa5           inc    word ptr [0xa51a]       
  0324c  e8cd0f             call   0x421c                  
  0324f  813eedaa0000       cmp    word ptr [0xaaed], 0    
  03255  7403               je     0x325a                  
  03257  e9bb00             jmp    0x3315                  
  0325a  813e2fa96300       cmp    word ptr [0xa92f], 0x63 
  03260  7503               jne    0x3265                  
  03262  e9b000             jmp    0x3315                  
  03265  813e2fa90200       cmp    word ptr [0xa92f], 2    
  0326b  7503               jne    0x3270                  
  0326d  e9a500             jmp    0x3315                  

;==============================================================================
; OVERLAY 7 entry ovl7_fn_30f0   (file 0x124C0, loads to seg-offset 0x30F0)
;==============================================================================

ovl7_fn_30f0:
  030f0  55                 push   bp                      
  030f1  8bec               mov    bp, sp                  
  030f3  83ec08             sub    sp, 8                   
  030f6  e83f16             call   0x4738                  
  030f9  b8401f             mov    ax, 0x1f40              
  030fc  50                 push   ax                      
  030fd  b8680b             mov    ax, 0xb68               
  03100  50                 push   ax                      
  03101  ff367aa6           push   word ptr [0xa67a]       
  03105  e8e5e6             call   0x17ed                  
  03108  83c406             add    sp, 6                   
  0310b  09c0               or     ax, ax                  
  0310d  7507               jne    0x3116                  
  0310f  b80f27             mov    ax, 0x270f              
  03112  8be5               mov    sp, bp                  
  03114  5d                 pop    bp                      
  03115  c3                 ret                            
  03116  b8680b             mov    ax, 0xb68               
  03119  05803e             add    ax, 0x3e80              
  0311c  8906cea6           mov    word ptr [0xa6ce], ax   
  03120  b86c88             mov    ax, 0x886c              
  03123  8906d0a6           mov    word ptr [0xa6d0], ax   
  03127  b8401f             mov    ax, 0x1f40              
  0312a  50                 push   ax                      
  0312b  ff36cea6           push   word ptr [0xa6ce]       
  0312f  b8bf02             mov    ax, 0x2bf               
  03132  50                 push   ax                      
  03133  e8b7e6             call   0x17ed                  
  03136  83c406             add    sp, 6                   
  03139  09c0               or     ax, ax                  
  0313b  7507               jne    0x3144                  
  0313d  b80f27             mov    ax, 0x270f              
  03140  8be5               mov    sp, bp                  
  03142  5d                 pop    bp                      
  03143  c3                 ret                            
  03144  b8b80b             mov    ax, 0xbb8               
  03147  50                 push   ax                      
  03148  ff36d0a6           push   word ptr [0xa6d0]       
  0314c  b8c202             mov    ax, 0x2c2               
  0314f  50                 push   ax                      
  03150  e89ae6             call   0x17ed                  
  03153  83c406             add    sp, 6                   
  03156  09c0               or     ax, ax                  
  03158  7507               jne    0x3161                  
  0315a  b80f27             mov    ax, 0x270f              
  0315d  8be5               mov    sp, bp                  
  0315f  5d                 pop    bp                      
  03160  c3                 ret                            
  03161  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  03165  05d007             add    ax, 0x7d0               
  03168  890699ab           mov    word ptr [0xab99], ax   
  0316c  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  03170  05f601             add    ax, 0x1f6               
  03173  89069ba9           mov    word ptr [0xa99b], ax   
  03177  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  0317b  050b02             add    ax, 0x20b               
  0317e  89069da9           mov    word ptr [0xa99d], ax   
  03182  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  03186  052002             add    ax, 0x220               
  03189  89069fa9           mov    word ptr [0xa99f], ax   
  0318d  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  03191  053502             add    ax, 0x235               
  03194  8906a1a9           mov    word ptr [0xa9a1], ax   
  03198  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  0319c  054a02             add    ax, 0x24a               
  0319f  8906a3a9           mov    word ptr [0xa9a3], ax   
  031a3  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  031a7  057402             add    ax, 0x274               
  031aa  8906a5a9           mov    word ptr [0xa9a5], ax   
  031ae  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  031b2  05c402             add    ax, 0x2c4               
  031b5  8906a7a9           mov    word ptr [0xa9a7], ax   
  031b9  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  031bd  051403             add    ax, 0x314               
  031c0  89068fa9           mov    word ptr [0xa98f], ax   
  031c4  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  031c8  052803             add    ax, 0x328               
  031cb  890691a9           mov    word ptr [0xa991], ax   
  031cf  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  031d3  053c03             add    ax, 0x33c               
  031d6  890693a9           mov    word ptr [0xa993], ax   
  031da  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  031de  055003             add    ax, 0x350               
  031e1  890695a9           mov    word ptr [0xa995], ax   
  031e5  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  031e9  056403             add    ax, 0x364               
  031ec  890697a9           mov    word ptr [0xa997], ax   
  031f0  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  031f4  057803             add    ax, 0x378               
  031f7  890699a9           mov    word ptr [0xa999], ax   
  031fb  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  031ff  058c03             add    ax, 0x38c               
  03202  890629a9           mov    word ptr [0xa929], ax   
  03206  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  0320a  05c403             add    ax, 0x3c4               
  0320d  89062ba9           mov    word ptr [0xa92b], ax   
  03211  8b06d0a6           mov    ax, word ptr [0xa6d0]   
  03215  05fc03             add    ax, 0x3fc               
  03218  89062da9           mov    word ptr [0xa92d], ax   
  0321c  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03220  051c1a             add    ax, 0x1a1c              
  03223  be400b             mov    si, 0xb40               
  03226  8904               mov    word ptr [si], ax       
  03228  8b06cea6           mov    ax, word ptr [0xa6ce]   
  0322c  05341a             add    ax, 0x1a34              
  0322f  be420b             mov    si, 0xb42               
  03232  8904               mov    word ptr [si], ax       
  03234  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03238  05601a             add    ax, 0x1a60              
  0323b  be440b             mov    si, 0xb44               
  0323e  8904               mov    word ptr [si], ax       
  03240  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03244  05841a             add    ax, 0x1a84              
  03247  be480b             mov    si, 0xb48               
  0324a  8904               mov    word ptr [si], ax       
  0324c  be460b             mov    si, 0xb46               
  0324f  8904               mov    word ptr [si], ax       
  03251  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03255  05a01a             add    ax, 0x1aa0              
  03258  be4a0b             mov    si, 0xb4a               
  0325b  8904               mov    word ptr [si], ax       
  0325d  8b06cea6           mov    ax, word ptr [0xa6ce]   
  03261  05b41a             add    ax, 0x1ab4              
  03264  be4e0b             mov    si, 0xb4e               
  03267  8904               mov    word ptr [si], ax       
  03269  be4c0b             mov    si, 0xb4c               
  0326c  8904               mov    word ptr [si], ax       

;==============================================================================
; OVERLAY 8  (file 0x172E0-0x18000) - TEXT / MESSAGE DATA (not code)
;==============================================================================
  170f0  db "B:M"
  170f4  db "A:M"
  170fa  db "Not found"
  17106  db "One moment, please"
  17119  db "Formation: "
  17125  db "CNF"
  1712c  db "presented by"
  17144  db "Game not saved!"
  17154  db "<C>ontinue with game or <Q>uit?"
  1718e  db "3.0"
  17192  db "2.0"
  171b2  db "CNF"
  171b6  db "Unable to use data disk."
  171cf  db "Press any key to continue."
  171ea  db "to back up"
  171f8  db "to "
  17202  db "You must use a formatted disk!"
  17221  db "Put WAR disk in drive, press any key"
  1724a  db "WarW"
  1724f  db "Welcome to"
  1725a  db "The Ancient Art of War."
  17279  db "Thank You"
  17283  db "Sorry, that answer is wrong."
  172af  db "Yes"
  172b6  db "End"
  172ba  db "WON"
  172be  db "LOST"
  172c3  db "ENEMY"
  172c9  db "NORTH"
  172cf  db "SOUTH"
  172d5  db "WEST"
  172da  db "EAST"
  172df  db "<--"
  172e3  db "-->"
  172e7  db "End "
  172ec  db " Knight"
  172f4  db " Barb."
  172fb  db " Archer"
  17303  db " Spy"
  17308  db "View"
  1730d  db "Time ("
  17314  db "faster"
  1731b  db "fast"
  17320  db "medium"
  17327  db "slow"
  1732c  db ")  "
  17330  db "Clear"
  17336  db "Move "
  1733c  db "Join "
  17342  db "Zoom"
  17347  db "SPY:"
  1734c  db "End"
  17350  db "Speed "
  17357  db "End"
  1735b  db " in a "
  17362  db "north"
  17368  db "south"
  1736e  db "west"
  17377  db "east"
  1737c  db "End"
  17380  db "Enemy has "
  1738b  db " men"
  17390  db "you have "
  1739a  db " men"
  173a5  db " me"
  173a9  db " ma"
  173ad  db " men"
  173b2  db " man"
  173f0  db "NO CORE$"
  1744a  db "<<0"
  17485  db "<<<"
  174a6  db "<<?"
  174aa  db "<<<<"
  174b2  db "<<?"
  174b6  db "<<?"
  174bc  db "<<<"
  174ca  db "<<<<<<"
  175b9  db "0. "
  175bd  db "!"#"
  175c1  db "$%&21"
  175f0  db "OPQ"
  17642  db "DPQZSJHIU 2:94 EBWF NVSSZInsert disk in drive"   ; +1 cipher -> "COPYRIGHT 1983 DAVE MURRY..."
  17670  db "Disk write protected"
  17685  db ", <R>etry or <Q>uit"
  1769d  db "03<?"
  176b1  db "0xx00"
  176b9  db "lll"
  176d9  db "8l8v"
  176ea  db "0```0"
  1770e  db "00`"
  17731  db "0p0000"
  17765  db "000"
  17786  db "00`"
  177ba  db "ff|ff"
  177ca  db "lfffl"
  177d2  db "bhxhb"
  177da  db "bhxh`"
  177f1  db "x00000x"
  17802  db "flxlf"
  1780a  db "```bf"
  1782a  db "ff|``"
  1783a  db "ff|lf"
  1784b  db "0000x"
  1786b  db "l88l"
  17874  db "x00x"
  17881  db "x`````x"
  178ba  db "``|ff"
  178d9  db "8l`"
  178ea  db "`lvff"
  178f3  db "p000x"
  17902  db "`flxl"
  17909  db "p00000x"
  1792c  db "ff|`"
  1793c  db "vf`"
  1794a  db "0|004"
  1796c  db "l8l"
  179c5  db "THE ANCIENT ART OF WAR COPYRIGHT 1987 BRODERBUND SOFTWARE"
  17a3e  db "5-."
  17bdc  db "On page "
  17be5  db ", what is word "
  17bf7  db "       in paragraph "
