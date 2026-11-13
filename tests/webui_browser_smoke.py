#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-or-later
"""Optional real-browser test: requires Chromium, Weston, ffmpeg and websocket-client."""
import argparse, base64, json, os, pathlib, re, shutil, socket, struct, subprocess, tempfile, time, traceback, urllib.request, zlib
import websocket

parser=argparse.ArgumentParser(description=__doc__)
parser.add_argument('--player',required=True)
parser.add_argument('--browser',default=shutil.which('chromium') or '/usr/lib/chromium/chromium')
parser.add_argument('--weston',default=shutil.which('weston') or '/usr/bin/weston')
parser.add_argument('--output',required=True)
args=parser.parse_args()
fixture=tempfile.TemporaryDirectory(prefix='lofibox-browser-')
base=pathlib.Path(fixture.name)
out=pathlib.Path(args.output).resolve();out.mkdir(parents=True,exist_ok=True)
music=base/'home/Music';music.mkdir(parents=True,exist_ok=True)
runtime=base/'runtime';runtime.mkdir(exist_ok=True);runtime.chmod(0o700)
bin_dir=base/'bin';bin_dir.mkdir(exist_ok=True)
(bin_dir/'aplay').write_text('#!/usr/bin/python3\nimport sys,time\nwhile True:\n data=sys.stdin.buffer.read(4096)\n if not data:break\n time.sleep(len(data)/384000)\n')
(bin_dir/'curl').write_text("#!/usr/bin/python3\nimport json,sys,urllib.parse\nurl=sys.argv[-1]\nif 'wikipedia.org/' not in url:sys.exit(1)\ntitle=urllib.parse.parse_qs(urllib.parse.urlparse(url).query).get('titles',[''])[0]\nprint(json.dumps({'query':{'pages':{'1':{'extract':'Fixture summary: '+title}}}},separators=(',',':')))\n")
for name in ['aplay','curl']:(bin_dir/name).chmod(0o755)

def chunk(kind,payload):return struct.pack('!I',len(payload))+kind+payload+struct.pack('!I',zlib.crc32(kind+payload))
cover=base/'cover.png'
cover.write_bytes(b'\x89PNG\r\n\x1a\n'+chunk(b'IHDR',struct.pack('!IIBBBBB',16,16,8,2,0,0,0))+chunk(b'IDAT',zlib.compress((b'\0'+b'\x80\x40\x10'*16)*16))+chunk(b'IEND',b''))
fixtures=[('One','Alpha','Shared Album',True),('Two','Alpha','Shared Album',True),('Three','Beta','Shared Album',True),('Quote Track',"C:\\Band '中文",'A\\B & "Quotes" \'作品',True),('No Lyrics','Gamma','Silent Album',False)]
for index,(title,artist,album,lyrics) in enumerate(fixtures):
    file=music/(artist+' - '+title+'.flac')
    if not file.exists():
        command=['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i',f'sine=frequency={400+index*100}:duration=180','-i',str(cover),'-map','0:a','-map','1:v','-c:a','flac','-c:v','copy','-disposition:v','attached_pic','-metadata','title='+title,'-metadata','artist='+artist,'-metadata','album='+album,'-metadata','genre=Jazz']
        if lyrics:command+=['-metadata','lyrics=[00:00.00]First fixture line\n[00:02.00]Second fixture line']
        subprocess.run(command+[str(file)],check=True)

def port():
    with socket.socket() as s:s.bind(('127.0.0.1',0));return s.getsockname()[1]
http_port,debug_port=port(),port()
env=os.environ.copy();env.pop('LD_PRELOAD',None);env.pop('DISPLAY',None)
env.update(HOME=str(base/'home'),XDG_RUNTIME_DIR=str(runtime),XDG_CONFIG_HOME=str(base/'config'),XDG_DATA_HOME=str(base/'data'),XDG_CACHE_HOME=str(base/'cache'),XDG_STATE_HOME=str(base/'state'),WAYLAND_DISPLAY='lofibox-headless',WAYLAND_DEBUG='client',LOFIBOX_MEDIA_ROOT=str(music),LOFIBOX_WEBUI='1',LOFIBOX_WEBUI_BIND='127.0.0.1',LOFIBOX_WEBUI_PORT=str(http_port),PATH=str(bin_dir)+':'+env['PATH'])
exe=str(pathlib.Path(args.player).resolve())
logs=[];processes=[];report={'checks':{},'host_only':True,'fixture':'real Wayland player, Weston headless pixman, Chromium headless, paced PCM sink'}

def launch(args,name,process_env=env):
    log=open(out/(name+'.log'),'w');logs.append(log)
    p=subprocess.Popen(args,env=process_env,stdout=log,stderr=log);processes.append(p);return p
def until(fn,seconds=8):
    deadline=time.monotonic()+seconds;last=None
    while time.monotonic()<deadline:
        try:
            result=fn()
            if result:return result
        except Exception as error:last=error
        time.sleep(.05)
    raise AssertionError('condition timed out: '+str(last))
def http(path='/api/runtime/snapshot'):
    with urllib.request.urlopen(f'http://127.0.0.1:{http_port}'+path,timeout=3) as r:return json.load(r)
def cli(*args):
    p=subprocess.run([exe,*args,'--json'],env=env,text=True,capture_output=True,timeout=10)
    assert p.returncode==0,(p.stdout,p.stderr)
    return json.loads(p.stdout)

class Cdp:
    def __init__(self,url):
        self.ws=websocket.create_connection(url,timeout=8,suppress_origin=True)
        self.id=0;self.exceptions=[]
    def call(self,method,params=None):
        self.id+=1;request=self.id
        self.ws.send(json.dumps({'id':request,'method':method,'params':params or {}}))
        while True:
            msg=json.loads(self.ws.recv())
            if msg.get('method')=='Runtime.exceptionThrown':self.exceptions.append(msg['params'])
            if msg.get('id')==request:
                assert 'error' not in msg,msg
                return msg.get('result',{})
    def js(self,expression):
        result=self.call('Runtime.evaluate',{'expression':expression,'returnByValue':True,'awaitPromise':True})
        assert 'exceptionDetails' not in result,result
        return result.get('result',{}).get('value')
    def click(self,selector=None,expression=None,ratio=.5):
        self.call('Page.bringToFront')
        target=expression or 'document.querySelector('+json.dumps(selector)+')'
        rect=self.js('(()=>{const e='+target+';if(!e)return null;e.scrollIntoView({block:"center"});const r=e.getBoundingClientRect();return {x:r.x+r.width/2,y:r.y+r.height*'+str(ratio)+'}})()')
        assert rect,(selector,expression)
        self.call('Input.dispatchMouseEvent',{'type':'mousePressed','button':'left','clickCount':1,**rect})
        self.call('Input.dispatchMouseEvent',{'type':'mouseReleased','button':'left','clickCount':1,**rect})
    def screenshot(self,name):
        data=self.call('Page.captureScreenshot',{'format':'png','captureBeyondViewport':False})['data']
        (out/(name+'.png')).write_bytes(base64.b64decode(data))
    def drag(self,selector,start=.8,end=.2,touch=False):
        self.call('Page.bringToFront')
        rect=self.js('(()=>{const e=document.querySelector('+json.dumps(selector)+');e.scrollIntoView({block:"center"});const r=e.getBoundingClientRect();return {x:r.x+r.width/2,y:r.y,h:r.height}})()')
        def point(ratio):return {'x':rect['x'],'y':rect['y']+rect['h']*ratio}
        if touch:
            self.call('Emulation.setTouchEmulationEnabled',{'enabled':True,'maxTouchPoints':1})
            self.call('Input.dispatchTouchEvent',{'type':'touchStart','touchPoints':[point(start)]})
        else:self.call('Input.dispatchMouseEvent',{'type':'mousePressed','button':'left','buttons':1,'clickCount':1,**point(start)})
        for i in range(1,6):
            location=point(start+(end-start)*i/5)
            if touch:self.call('Input.dispatchTouchEvent',{'type':'touchMove','touchPoints':[location]})
            else:self.call('Input.dispatchMouseEvent',{'type':'mouseMoved','button':'left','buttons':1,**location})
        if touch:self.call('Input.dispatchTouchEvent',{'type':'touchEnd','touchPoints':[]})
        else:self.call('Input.dispatchMouseEvent',{'type':'mouseReleased','button':'left','clickCount':1,**point(end)})
        self.js('_commandChain')

def check(name,fn):
    try:fn();report['checks'][name]='passed'
    except Exception as error:
        report['checks'][name]=repr(error)
        traceback.print_exc()
        if cdp:
            report.setdefault('failed_states',{})[name]={'browser':cdp.js('state'),'server':http()}
    print(name,report['checks'][name],flush=True)

browser=None;cdp=None;second=None
try:
    weston=launch([args.weston,'--backend=headless-backend.so','--use-pixman','--width=640','--height=480','--socket=lofibox-headless','--idle-time=0','--no-config'],'weston')
    until(lambda:(runtime/'lofibox-headless').exists())
    player=launch([exe],'player')
    until(lambda:http()['library']['ready'])
    tracks=http('/api/library/tracks');report['tracks']=tracks
    ids={t['title']:t['id'] for t in tracks};assert len(ids)==5,tracks
    cli('play','--id',str(ids['One']))
    until(lambda:http()['lyrics']['available'])
    browser_env=dict(env);browser_env.pop('WAYLAND_DEBUG',None)
    browser=launch([args.browser,'--headless','--no-sandbox','--disable-gpu','--disable-dev-shm-usage','--disable-background-networking','--host-resolver-rules=MAP * ~NOTFOUND, EXCLUDE 127.0.0.1','--no-first-run','--no-default-browser-check','--no-proxy-server','--user-data-dir='+str(base/'chrome-profile'),'--remote-debugging-port='+str(debug_port),'--remote-debugging-address=127.0.0.1','about:blank'],'chromium',browser_env)
    def target():
        with urllib.request.urlopen(f'http://127.0.0.1:{debug_port}/json/list',timeout=2) as r:
            return next((x['webSocketDebuggerUrl'] for x in json.load(r) if x['type']=='page'),None)
    cdp=Cdp(until(target));cdp.call('Page.enable');cdp.call('Runtime.enable')
    cdp.call('Emulation.setDeviceMetricsOverride',{'width':390,'height':844,'deviceScaleFactor':1,'mobile':True})
    cdp.call('Page.navigate',{'url':f'http://127.0.0.1:{http_port}'})
    until(lambda:cdp.js('typeof state!=="undefined" && state.playback.title==="One"'))
    cdp.screenshot('now-playing')
    report['checks']['wayland_start_and_browser_load']='passed'
    def empty_delta_artwork():
        assert cdp.js('''(()=>{const saved=state.playback;
            state.playback={...saved,status:'empty'};renderPlaying();
            const cleared=document.querySelectorAll('#np-artwork img').length===0;
            state.playback=saved;renderPlaying();return cleared})()''')
    check('empty_delta_with_previous_id_clears_artwork',empty_delta_artwork)
    cdp.js('window.mutations={eq:0,lyrics:0}; new MutationObserver(m=>mutations.eq+=m.length).observe(document.getElementById("eq-bands"),{childList:true,subtree:true}); new MutationObserver(m=>mutations.lyrics+=m.length).observe(document.getElementById("np-lyrics"),{childList:true,subtree:true})')
    time.sleep(1);report['mutations_per_second']=cdp.js('mutations')
    def transport():
        cdp.click('#btn-play');until(lambda:http()['playback']['status']=='paused')
        cdp.click('#btn-play');until(lambda:http()['playback']['status']=='playing')
    check('play_pause_clicks',transport)
    cdp.click('nav [data-page="eq"]');time.sleep(.35)
    def slider():
        cdp.click('.eq-band[data-index="2"] .eq-slider',ratio=.2)
        until(lambda:http()['eq']['bands'][2]>0,seconds=1.2)
    check('eq_slider_after_live_updates',slider)
    def toggle():
        initial=http()['eq']['enabled']
        cdp.click('#eq-toggle');until(lambda:http()['eq']['enabled']!=initial)
        cdp.click('#eq-toggle');until(lambda:http()['eq']['enabled']==initial,seconds=1.2)
    check('eq_enable_disable',toggle)
    def rapid_toggle():
        initial=http()['eq']['enabled']
        for _ in range(5):cdp.click('#eq-toggle')
        cdp.js('_commandChain')
        until(lambda:http()['eq']['enabled']!=initial)
        until(lambda:cdp.js('_eqTogglePending===null && state.eq.enabled==='+('false' if initial else 'true')),seconds=1.2)
        assert cdp.js('document.getElementById("eq-toggle").textContent')==('OFF' if initial else 'ON')
    check('rapid_eq_toggle_preserves_last_user_intent',rapid_toggle)
    def final_slider_values():
        cdp.drag('.eq-band[data-index="3"] .eq-slider')
        until(lambda:http()['eq']['bands'][3]==7)
        cdp.drag('.eq-band[data-index="4"] .eq-slider',touch=True)
        until(lambda:http()['eq']['bands'][4]==7)
        track=http()['playback']['current_track_id']
        cdp.js("document.querySelector('.eq-band[data-index=\"4\"] .eq-slider').focus()")
        cdp.call('Input.dispatchKeyEvent',{'type':'keyDown','key':'ArrowUp','code':'ArrowUp'})
        cdp.call('Input.dispatchKeyEvent',{'type':'keyUp','key':'ArrowUp','code':'ArrowUp'})
        until(lambda:http()['eq']['bands'][4]==8)
        assert http()['playback']['current_track_id']==track
    check('mouse_touch_keyboard_and_final_drag_value',final_slider_values)
    cdp.screenshot('eq')
    cdp.click('nav [data-page="library"]');until(lambda:cdp.js('document.querySelectorAll("#lib-list .q-row").length>=3'))
    def album():
        cdp.click(expression='Array.from(document.querySelectorAll("#lib-list .q-row")).find(e=>e.querySelector(".q-title")?.textContent==="Shared Album" && e.querySelector(".q-meta")?.textContent==="Alpha")')
        until(lambda:cdp.js('document.querySelectorAll("#lib-list [data-track-id]").length>0'))
        assert cdp.js('document.querySelectorAll("#lib-list [data-track-id]").length')==2,cdp.js('document.getElementById("lib-list").innerText')
    check('same_name_albums_keep_artist_filter',album)
    def album_play_and_cache():
        cdp.click('.album-header .album-play-all');cdp.js('_commandChain')
        until(lambda:http()['queue']['count']==2)
        assert {item['artist'] for item in http()['queue']['items']}=={'Alpha'}
        until(lambda:cdp.js('document.querySelector("#enrich-top .enrich-summary")?.textContent.includes("Alpha")'))
        cdp.js('libGoHome()');time.sleep(.05)
        album()
        until(lambda:cdp.js('document.querySelector("#enrich-top .enrich-summary")?.textContent.includes("Alpha")'))
    check('album_play_all_and_cached_enrichment',album_play_and_cache)
    cdp.js('libGoHome()');time.sleep(.1)
    def quotes():
        title=fixtures[3][2]
        cdp.click(expression='Array.from(document.querySelectorAll("#lib-list .q-row")).find(e=>e.querySelector(".q-title")?.textContent==='+json.dumps(title)+')')
        until(lambda:cdp.js('document.querySelectorAll("#lib-list [data-track-id]").length')==1,seconds=1.2)
    check('quoted_backslash_unicode_album_navigation',quotes)
    def quoted_play():
        cdp.click('.album-header .album-play-all');cdp.js('_commandChain')
        until(lambda:http()['playback']['current_track_id']==ids['Quote Track'])
        assert http()['queue']['count']==1
        until(lambda:cdp.js('document.querySelector("#enrich-top .enrich-summary")?.textContent.includes("作品")'))
    check('quoted_unicode_album_play_all_context',quoted_play)
    cdp.screenshot('library')
    report['large_library']=cdp.js('(()=>{libBreadcrumb=[];libView="tracks";libSearch="";libTracks=Array.from({length:10000},(_,i)=>({id:i+100,title:"Track "+i,artist:"Artist",album:"Album",duration_seconds:180}));libTracksLoaded=true;libAlbumsLoaded=true;_libContentKey="";const started=performance.now();renderLibContent();const result={render_ms:performance.now()-started,rows:document.querySelectorAll("#lib-list [data-track-id]").length,nav_bottom:document.querySelector("nav.tabs").getBoundingClientRect().bottom,viewport:innerHeight,document_height:document.documentElement.scrollHeight};return result})()')
    cdp.screenshot('large-library')
    def library_pages_and_search():
        assert report['large_library']['rows']==100
        cdp.click('[data-lib-action="more"]')
        until(lambda:cdp.js('document.querySelectorAll("#lib-list [data-track-id]").length')==200)
        cdp.click('#lib-search-input')
        cdp.call('Input.insertText',{'text':'Track 9999'})
        until(lambda:cdp.js('document.querySelectorAll("#lib-list [data-track-id]").length')==1)
        assert cdp.js('document.querySelector("#lib-list .q-title").textContent')=='Track 9999'
    check('large_library_load_more_and_full_catalog_search',library_pages_and_search)
    def pages_and_queue():
        for page in ['queue','more','eq','library','now-playing']:
            cdp.click('nav [data-page="'+page+'"]')
            assert cdp.js('document.querySelector(".page.active").id')=='page-'+page
            assert cdp.js('document.querySelector("nav.tabs").getBoundingClientRect().bottom<=innerHeight+1'),page
            assert cdp.js('document.documentElement.scrollHeight<=innerHeight+1'),page
        cli('play','--id',str(ids['One']))
        cdp.click('nav [data-page="queue"]')
        cli('queue','jump','0')
        expected=http()['playback']['title']
        until(lambda:cdp.js('document.querySelector("#queue-items .q-row.active .q-title")?.textContent')==expected,seconds=1.2)
        cdp.click('nav [data-page="more"]')
        cli('runtime','settings','apply','ALSA','LAN_ONLY','15M')
        until(lambda:cdp.js('document.getElementById("more-settings").innerText.includes("15M")'),seconds=1.2)
        assert cdp.js('document.querySelector("main").scrollHeight>document.querySelector("main").clientHeight')
        cdp.js('document.querySelector("main").scrollTop=document.querySelector("main").scrollHeight')
        assert cdp.js('document.querySelector("nav.tabs").getBoundingClientRect().bottom<=innerHeight+1')
        cdp.js('document.querySelector("main").scrollTop=0')
        cdp.screenshot('more')
    check('five_tabs_and_external_queue_settings_updates',pages_and_queue)
    def timer_controls():
        cdp.click('nav [data-page="more"]')
        assert cdp.js('document.querySelector("#screen-timer")===null') # Wayland uses compositor power management.
        cdp.js('(()=>{const e=document.querySelector("#sleep-timer");e.value="30M";e.dispatchEvent(new Event("change",{bubbles:true}))})()')
        until(lambda:http()['settings']['sleep_timer']=='30M')
        cli('runtime','settings','apply','','','OFF')
        until(lambda:cdp.js('document.querySelector("#sleep-timer").value==="OFF"'))
        result=cdp.js('sendCmd("SetTimers",{screen_timeout:"30S"})')
        assert result and result['accepted'] and not result['applied'],result
        assert http()['settings']['screen_timeout']=='OFF'
        cdp.js('window.timerPlaybackKeys=[];window.originalTimerSendCmd=sendCmd;sendCmd=(action,params)=>{if(["Next","Previous","Toggle"].includes(action))timerPlaybackKeys.push(action);return originalTimerSendCmd(action,params)};document.querySelector("#sleep-timer").focus()')
        for key,code in [('ArrowRight',39),(' ',32)]:
            cdp.call('Input.dispatchKeyEvent',{'type':'keyDown','key':key,'windowsVirtualKeyCode':code,'nativeVirtualKeyCode':code})
            cdp.call('Input.dispatchKeyEvent',{'type':'keyUp','key':key,'windowsVirtualKeyCode':code,'nativeVirtualKeyCode':code})
        assert cdp.js('timerPlaybackKeys.length')==0
        cdp.js('sendCmd=originalTimerSendCmd')
    check('timer_browser_changes_external_cancel_and_wayland_capability',timer_controls)
    cdp.click('nav [data-page="now-playing"]')
    def clear_lyrics():
        cli('play','--id',str(ids['No Lyrics']));until(lambda:not http()['lyrics']['available'])
        until(lambda:cdp.js('getComputedStyle(document.getElementById("np-lyrics")).display==="none"'),seconds=1.2)
    check('lyrics_clear_on_track_without_lyrics',clear_lyrics)
    cdp.screenshot('empty-lyrics')
    def reconnect():
        cdp.js('ws.close()')
        until(lambda:cdp.js('document.querySelector("#conn-status .status-dot").classList.contains("offline")'))
        cdp.click('nav [data-page="more"]')
        assert cdp.js('document.querySelector("#conn-status .status-dot").classList.contains("offline")')
        until(lambda:cdp.js('ws.readyState===WebSocket.OPEN && document.querySelector("#conn-status .status-dot").classList.contains("online")'),seconds=5)
    check('websocket_reconnect_and_connection_indicator',reconnect)
    def two_tabs():
        global second
        target_id=cdp.call('Target.createTarget',{'url':f'http://127.0.0.1:{http_port}'})['targetId']
        def second_url():
            with urllib.request.urlopen(f'http://127.0.0.1:{debug_port}/json/list',timeout=2) as r:
                return next((x['webSocketDebuggerUrl'] for x in json.load(r) if x['id']==target_id),None)
        second=Cdp(until(second_url));second.call('Runtime.enable');second.call('Page.enable')
        until(lambda:second.js('typeof state!=="undefined" && ws.readyState===WebSocket.OPEN'))
        cdp.click('nav [data-page="now-playing"]')
        cdp.call('Target.activateTarget',{'targetId':target_id})
        until(lambda:cdp.js('document.hidden'))
        until(lambda:cdp.js('_spectrumFrame===0'))
        cli('pause')
        until(lambda:cdp.js('state.playback.status==="paused"') and second.js('state.playback.status==="paused"'))
        cli('resume')
        until(lambda:cdp.js('state.playback.status==="playing"') and second.js('state.playback.status==="playing"'))
        second.ws.close();second=None
        cdp.call('Target.closeTarget',{'targetId':target_id});cdp.call('Page.bringToFront')
    check('two_live_browser_tabs_and_hidden_animation_suspension',two_tabs)
    report['console_exceptions']=cdp.exceptions
    assert not cdp.exceptions,cdp.exceptions
    report['wayland_commits']=len(re.findall(r'-> wl_surface@\d+\.commit\(\)',(out/'player.log').read_text()))
    cli('runtime','settings','apply','','','1S')
    player.wait(timeout=3);assert player.returncode==0
    report['checks']['sleep_timer_stops_and_exits_player']='passed'
    def restart():
        until(lambda:cdp.js('document.querySelector("#conn-status .status-dot").classList.contains("offline")'))
        restarted=launch([exe],'player-restart')
        until(lambda:http()['library']['ready'])
        cdp.click('nav [data-page="now-playing"]')
        until(lambda:cdp.js('ws.readyState===WebSocket.OPEN && state.playback.status==="empty"'),seconds=5)
        assert cdp.js('document.querySelectorAll("#np-artwork img").length')==0
        cli('runtime','shutdown');restarted.wait(timeout=3);assert restarted.returncode==0
    check('player_restart_reconnect_and_stale_artwork_clear',restart)
except Exception as error:
    report['fatal']=repr(error)
    if cdp:
        try:
            report['fatal_browser_state']=cdp.js('({url:location.href,ready:document.readyState,body:document.body.innerText.slice(0,2000),state:typeof state==="undefined"?null:state})')
            report['console_exceptions']=cdp.exceptions
            cdp.screenshot('failure')
        except Exception as diagnostic_error:report['diagnostic_error']=repr(diagnostic_error)
    traceback.print_exc()
finally:
    if second:second.ws.close()
    if cdp:cdp.ws.close()
    for process in reversed(processes):
        if process.poll() is None:
            process.terminate()
            try:process.wait(timeout=3)
            except subprocess.TimeoutExpired:process.kill();process.wait()
    for log in logs:log.close()
    fixture.cleanup()
    (out/'report.json').write_text(json.dumps(report,indent=2,ensure_ascii=False))
    print(json.dumps(report,indent=2,ensure_ascii=False))
raise SystemExit(1 if report.get('fatal') or any(value!='passed' for value in report['checks'].values()) else 0)
