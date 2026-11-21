# Random test for dictionaries. It writes random El programs that use
# dictionaries (set, update, remove, copy, nesting, functions, comparisons),
# works out what they should print with a small model in Python, runs them with
# elvm and compares the two.
#
#   python3 tests/fuzz/fuzz-dictionary.py compiler/elvm 500 [first-seed]
#
# Prints the number of programs that did not match. A failing program is
# written to /tmp/fz_fail_<seed>.ell.

import random, subprocess, sys, copy, os
KEYS=["a","b","c","d"]
STRS=["x","yy","hello world","z9",""]
def fmt_g(x):
    return format(x,'g')
def jq(s): return '"'+s.replace('\\','\\\\').replace('"','\\"').replace('\n','\\n')+'"'
def jtext(v):
    t=v[0]
    if t=='int': return str(v[1])
    if t=='float': return fmt_g(v[1])
    if t=='str': return jq(v[1])
    if t=='bool': return 'true' if v[1] else 'false'
    if t=='none': return 'NONE'
    return '{'+', '.join(jq(k)+': '+jtext(x) for k,x in v[1].items())+'}'
def ptext(v):
    if v[0]=='str': return v[1]
    return jtext(v)
def rnd_value(r):
    c=r.choice(['int','float','str','bool','none'])
    if c=='int': n=r.randint(-50,50); return ('int',n),str(n) if n>=0 else str(n)
    if c=='float': x=r.choice([0.5,1.5,-2.5,3.25,100.5]); return ('float',x),repr(x)
    if c=='str': s=r.choice(STRS); return ('str',s),jq(s)
    if c=='bool': b=r.random()<0.5; return ('bool',b),'true' if b else 'false'
    return ('none',),'NONE'
def gen(seed):
    r=random.Random(seed)
    D={'d':{},'e':{}}
    lines=['function ident(var dict x) =','  return x;','var dict d;','var dict e;']
    out=[]
    for _ in range(r.randint(15,40)):
        v=r.choice(['d','e']); w='e' if v=='d' else 'd'
        op=r.choice(['set','set','set','setnested','update','inc','remove','copyentry','copywhole','print','printentry','has','nestedset','nestedprint','newnested','literal','viafn','entryfn','cmp'])
        dv=D[v]
        if op=='set':
            k=r.choice(KEYS); val,txt=rnd_value(r)
            lines.append(f'{v}["{k}"] = {txt};'); dv[k]=val
        elif op=='literal':
            n=r.randint(0,3); ents={}
            parts=[]
            for _ in range(n):
                k=r.choice(KEYS); val,txt=rnd_value(r); ents[k]=val; parts.append(f'"{k}": {txt}')
            lines.append(f'{v} = {{'+', '.join(parts)+'};'); D[v]=ents
        elif op=='newnested':
            k=r.choice(KEYS); lines.append(f'{v}["{k}"] = {{}};'); dv[k]=('dict',{})
        elif op=='nestedset':
            ks=[k for k,x in dv.items() if x[0]=='dict']
            if not ks: continue
            k=r.choice(ks); k2=r.choice(KEYS); val,txt=rnd_value(r)
            lines.append(f'{v}["{k}"]["{k2}"] = {txt};'); dv[k][1][k2]=val
        elif op=='nestedprint':
            ks=[k for k,x in dv.items() if x[0]=='dict']
            if not ks: continue
            k=r.choice(ks); inner=dv[k][1]
            if inner and r.random()<0.6:
                k2=r.choice(list(inner)); lines.append(f'print {v}["{k}"]["{k2}"];'); out.append(ptext(inner[k2]))
            else:
                lines.append(f'print {v}["{k}"];'); out.append(jtext(dv[k]))
        elif op=='update' or op=='inc':
            ks=[k for k,x in dv.items() if x[0] in('int','float')]
            if not ks: continue
            k=r.choice(ks)
            if op=='inc':
                lines.append(f'{v}["{k}"]++;'); dv[k]=(dv[k][0],dv[k][1]+1)
            else:
                if r.random()<0.5:
                    n=r.randint(1,9); lines.append(f'{v}["{k}"] += {n};'); dv[k]=(dv[k][0],dv[k][1]+n)
                else:
                    x=r.choice([0.5,1.5]); lines.append(f'{v}["{k}"] -= {x};'); dv[k]=('float',dv[k][1]-x)
        elif op=='remove':
            k=r.choice(KEYS); lines.append(f'remove_key({v}, "{k}");'); dv.pop(k,None)
        elif op=='copyentry':
            if not D[w]: continue
            k2=r.choice(list(D[w])); k=r.choice(KEYS)
            lines.append(f'{v}["{k}"] = {w}["{k2}"];'); dv[k]=copy.deepcopy(D[w][k2])
        elif op=='viafn':
            lines.append(f'{v} = ident({w});'); D[v]=copy.deepcopy(D[w])
        elif op=='entryfn':
            k=r.choice(KEYS); lines.append(f'{v}["{k}"] = ident({w});'); dv[k]=('dict',copy.deepcopy(D[w]))
        elif op=='cmp':
            ks=[k for k,x in dv.items() if x[0] in('int','float','str')]
            if not ks: continue
            k=r.choice(ks); x=dv[k]
            if x[0]=='str':
                lit=r.choice(STRS); res=(x[1]==lit); cond=f'{v}["{k}"] == {jq(lit)}'
            else:
                n=r.randint(-5,5); res=(x[1]==n) if r.random()<0.5 else None
                if res is None: res=(x[1]>n); cond=f'{v}["{k}"] > {n}'
                else: cond=f'{v}["{k}"] == {n}'
            lines.append(f'if({cond}) =\n  print "T";\nelse =\n  print "F";'); out.append('T' if res else 'F')
        elif op=='copywhole':
            lines.append(f'{v} = {w};'); D[v]=copy.deepcopy(D[w])
        elif op=='print':
            lines.append(f'print {v};'); out.append(jtext(('dict',dv)))
        elif op=='printentry':
            if not dv: continue
            k=r.choice(list(dv)); lines.append(f'print {v}["{k}"];'); out.append(ptext(dv[k]))
        elif op=='has':
            k=r.choice(KEYS); lines.append(f'print has_key({v}, "{k}");'); out.append('true' if k in dv else 'false')
    lines.append('print d;'); out.append(jtext(('dict',D['d'])))
    lines.append('print e;'); out.append(jtext(('dict',D['e'])))
    return '\n'.join(lines)+'\n','\n'.join(out)+'\n'
elvm=sys.argv[1]; n=int(sys.argv[2]); start=int(sys.argv[3]) if len(sys.argv)>3 else 0
bad=0
for seed in range(start,start+n):
    prog,exp=gen(seed)
    open('/tmp/fz.ell','w').write(prog)
    p=subprocess.run([elvm,'/tmp/fz.ell'],capture_output=True,text=True,timeout=10)
    if p.stdout!=exp or p.returncode!=0:
        bad+=1
        if bad<=3:
            print("MISMATCH seed",seed,"rc",p.returncode); 
            open(f'/tmp/fz_fail_{seed}.ell','w').write(prog)
            a=exp.split('\n'); b=p.stdout.split('\n')
            for i,(x,y) in enumerate(zip(a,b)):
                if x!=y: print(" line",i,"expected:",x[:100],"| got:",y[:100]); break
            else: print(" length differs", len(a), len(b), p.stdout[-200:])
            print(" stderr:",p.stderr[:200])
print("programs:",n,"mismatches:",bad)
