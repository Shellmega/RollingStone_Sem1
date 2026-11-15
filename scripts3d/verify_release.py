"""Test a copied release with no developer PATH, using real GUI and numerical output."""
import argparse
import csv
import ctypes as C
import hashlib
import json
import os
from pathlib import Path
import subprocess
import time

def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('package',type=Path)
    parser.add_argument('--output',type=Path,required=True)
    parser.add_argument('--self-test',action='store_true')
    args=parser.parse_args();package=args.package.resolve();output=args.output.resolve()
    output.mkdir(parents=True,exist_ok=False)
    exe=package/'RollingStone3D.exe';config=package/'configs3d/batch_throughput.ini'
    root_screenshots={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in package.glob('screenshot*.png')}
    assert exe.is_file() and config.is_file()
    assert (package/'assets/fonts/NotoSansSC-Regular.ttf').is_file()
    env=os.environ.copy()
    for name in list(env):
        if name.upper().startswith(('VS','VSCMD','VCTOOLS','WINDOWSSDK','UNIVERSALCRT','UCRTVERSION')) or name.upper() in ('INCLUDE','LIB','LIBPATH','VCINSTALLDIR'):
            del env[name]
    env['PATH']=str(Path(env['WINDIR'])/'System32')
    def run(name,flags,expected=0,timeout=150):
        r=subprocess.run([str(exe),*map(str,flags)],cwd=package,env=env,stdout=subprocess.PIPE,stderr=subprocess.STDOUT,timeout=timeout)
        (output/(name+'.txt')).write_bytes(r.stdout)
        assert r.returncode==expected,(name,r.returncode,r.stdout[-1500:])
        return r.stdout.decode('utf-8',errors='replace')
    run('help',['--help'])
    run('invalid_scale',['--scale','nan','--headless'],1)
    run('invalid_size',['--size','1366x768junk','--headless'],1)
    run('missing_config',['--config',output/'不存在.ini','--headless'],1)
    common=['--config',config,'--batch','2','--seed','2026','--preview','0']
    run('headless',[*common,'--headless','--output',output/'中文 数值'])
    u=C.windll.user32;callback=C.WINFUNCTYPE(C.c_bool,C.c_void_p,C.c_ssize_t)
    u.GetWindowThreadProcessId.argtypes=[C.c_void_p,C.POINTER(C.c_ulong)]
    u.IsWindowVisible.argtypes=[C.c_void_p]
    u.PostMessageW.argtypes=[C.c_void_p,C.c_uint,C.c_size_t,C.c_ssize_t]
    with (output/'gui_stdout.txt').open('wb') as stream:
        command=[str(exe),*map(str,[*common,'--output',output/'中文 窗口','--duration','5','--size','1280x800','--scale','1','--shots',output/'截图','--input-log',output/'gui_input.txt'])]
        process=subprocess.Popen(command,cwd=package,env=env,stdout=stream,stderr=subprocess.STDOUT)
        start=time.monotonic();sent=False
        try:
            while process.poll() is None and time.monotonic()-start<40:
                if not sent and time.monotonic()-start>2:
                    @callback
                    def window(h,_):
                        nonlocal sent
                        pid=C.c_ulong();u.GetWindowThreadProcessId(h,C.byref(pid))
                        if pid.value==process.pid and u.IsWindowVisible(h):
                            u.PostMessageW(h,0x100,0x7B,0);u.PostMessageW(h,0x101,0x7B,0)
                            sent=True;return False
                        return True
                    u.EnumWindows(window,0)
                time.sleep(.05)
            process.wait(timeout=5);assert process.returncode==0,process.returncode
        finally:
            if process.poll() is None:process.terminate();process.wait(timeout=5)
    assert sent and (output/'截图/screenshot.png').is_file(),'GUI framebuffer not captured'
    log=(output/'gui_input.txt').read_text(encoding='utf-8')
    assert 'frame error:' not in log and 'phase=2 ' not in log,'preview 0 ignored or GUI frame error'
    def archive(parent):
        paths=[p for p in parent.glob('run_*') if p.is_dir() and p.suffix!='.pending']
        assert len(paths)==1,paths
        assert (paths[0]/'FILES_COMPLETE.txt').is_file()
        return paths[0]
    numerical=archive(output/'中文 数值');graphical=archive(output/'中文 窗口')
    def physical(folder):
        with (folder/'trials.csv').open(encoding='utf-8',newline='') as stream:
            return [{k:v for k,v in row.items() if not k.endswith('_seconds')} for row in csv.DictReader(stream)]
    assert len(physical(numerical))==2 and physical(numerical)==physical(graphical)
    for name in ['shape_vertices.csv','shape_faces.csv','trajectory.csv','voxel_probability.csv','collector_yz.csv','projection_xy.csv','projection_xz.csv']:
        assert (numerical/name).read_bytes()==(graphical/name).read_bytes(),name
    run('readonly',['--view-results',numerical,'--headless'])
    assert 'physics_updates=0' in (output/'readonly.txt').read_text()
    run('replay',['--replay',numerical,'--trial','1','--headless'])
    if args.self_test:run('self_test_22',['--self-test'],timeout=300)
    assert root_screenshots=={p.name:hashlib.sha256(p.read_bytes()).hexdigest() for p in package.glob('screenshot*.png')},'F12 wrote duplicate screenshots into package root'
    report=dict(passed=True,package=str(package),gui_headless_identical=True,chinese_paths=True,preview_zero_honoured=True,
                readonly_physics_updates=0,replay_pass=True,no_developer_environment=True,self_test_22=args.self_test,
                duplicate_capture_disabled=True,executable_sha256=hashlib.sha256(exe.read_bytes()).hexdigest(),screenshot=str(output/'截图/screenshot.png'))
    (output/'验收结果.json').write_text(json.dumps(report,ensure_ascii=False,indent=2),encoding='utf-8')
    print(json.dumps(report,ensure_ascii=False,indent=2),flush=True)

if __name__=='__main__':main()
