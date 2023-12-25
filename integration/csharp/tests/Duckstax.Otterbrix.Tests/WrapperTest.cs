namespace Duckstax.Otterbrix.Tests;

using Duckstax.EntityFramework.Otterbrix;

public class Tests
{
    private otterbrixWrapper wrapper { get; set; }

    [SetUp]
    public void Setup()
    {
        this.wrapper = new otterbrixWrapper();
    }

    [Test]
    public void Test1()
    {
        cursorWrapper cursor = this.wrapper.Execute("SELECT * FROM TestDatabase.TestCollection;");
        Assert.IsTrue(cursor != null);
    }
}