package com.jn.ffmpeg;

import org.junit.Test;

import java.lang.ref.PhantomReference;
import java.lang.ref.Reference;
import java.lang.ref.ReferenceQueue;
import java.lang.ref.SoftReference;
import java.lang.ref.WeakReference;

import static org.junit.Assert.*;

/**
 * Example local unit test, which will execute on the development machine (host).
 *
 * @see <a href="http://d.android.com/tools/testing">Testing documentation</a>
 */
public class ExampleUnitTest {
    @Test
    public void addition_isCorrect() {
        assertEquals(4, 2 + 2);
    }



    @Test
    public void testPhantomReference(){

        Object o = new Object();
        ReferenceQueue<Object> objectReferenceQueue = new ReferenceQueue<>();
        PhantomReference<Object> phantomReference = new PhantomReference<>(o,objectReferenceQueue);
        o = null;//去除强引用
        System.gc();
        try {
            Thread.sleep(200);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        System.out.println(phantomReference.get());//null
        System.out.println((Object) objectReferenceQueue.poll()); //hash值
    }
    @Test
    public void testWeakReference() throws InterruptedException {
        Object obj  = new Object();
        ReferenceQueue<Object> objectReferenceQueue = new ReferenceQueue<>();
        WeakReference<Object> weakReference = new WeakReference<>(obj,objectReferenceQueue);
        obj = null;//去掉强引用
        System.gc();
        Thread.sleep(500);
        //第一次GC在堆中扫到了没有任何可达变量引用的对象，就标记下来，第二次扫到直接回收
        System.out.println(weakReference.get());
        System.out.println(objectReferenceQueue.poll());


    }
    @Test
    public void testSoftReference(){


        Object object = new Object();
        SoftReference<Object> softReference = new SoftReference<>(object);
        object = null;//去掉强引用
        System.gc();
        try {
            Thread.sleep(200);
        } catch (InterruptedException e) {
            e.printStackTrace();
        }
        //在内存不足时,才会回收
        System.out.println("softReference"+softReference.get());

    }














}